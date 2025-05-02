#include "IPManager.h"
#include "Logger.h"
#include <windows.h>
#include <vector>
#include <iphlpapi.h>
#include <comdef.h>
#include <wbemidl.h>
#include <sstream>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wbemuuid.lib")

// Helper function to safely convert size_t to DWORD
DWORD SafeSizeToDword(size_t size) {
    if (size > MAXDWORD) {
        throw std::overflow_error("Size exceeds maximum DWORD value");
    }
    return static_cast<DWORD>(size);
}




DWORD RunNetshCommand(const std::wstring& command, const std::wstring& logFile) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = TRUE;

    HANDLE hStdOutRead, hStdOutWrite;
    CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0);
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;

    PROCESS_INFORMATION pi;
    std::wstring cmd = L"netsh " + command;

    if (!CreateProcess(nullptr, (LPWSTR)cmd.c_str(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        LogMessage(L"CreateProcess failed: " + std::to_wstring(GetLastError()), logFile);
        return GetLastError();
    }

    CloseHandle(hStdOutWrite);
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    DWORD bytesRead;
    std::wstring output;
    wchar_t buffer[1024];
    while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - sizeof(wchar_t), &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead / sizeof(wchar_t)] = L'\0';
        output += buffer;
    }

    LogMessage(L"Command: " + cmd + L"\nOutput: " + output, logFile);

    CloseHandle(hStdOutRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode;
}

void ResetAdapter(const std::wstring& adapterName) {
    std::wstring disableCommand = L"interface set interface name=\"" + adapterName + L"\" admin=disable";
    RunNetshCommand(disableCommand, L"netsh_reset.log");

    std::wstring enableCommand = L"interface set interface name=\"" + adapterName + L"\" admin=enable";
    RunNetshCommand(enableCommand, L"netsh_reset.log");
}

bool IsAdapterActive(const std::wstring& adapterName) {
    ULONG bufferSize = 0;
    GetIfTable(NULL, &bufferSize, FALSE);
    std::vector<BYTE> buffer(bufferSize);
    PMIB_IFTABLE pIfTable = (PMIB_IFTABLE)buffer.data();

    if (GetIfTable(pIfTable, &bufferSize, FALSE) == NO_ERROR) {
        for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
            std::wstring name = std::wstring(pIfTable->table[i].wszName);
            if (name == adapterName && pIfTable->table[i].dwOperStatus == IF_OPER_STATUS_OPERATIONAL) {
                return true;
            }
        }
    }
    return false;
}

DWORD SetAdapterToStaticIpViaRegistry(const std::wstring& adapterName, const std::wstring& ipAddress, const std::wstring& subnetMask, const std::wstring& gateway, const std::wstring& dns) {
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        LogMessage(L"CoInitialize failed: " + std::to_wstring(hr), L"service_error.log");
        return hr;
    }

    IWbemLocator* pLoc = nullptr;
    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hr)) {
        CoUninitialize();
        LogMessage(L"CoCreateInstance failed: " + std::to_wstring(hr), L"service_error.log");
        return hr;
    }

    IWbemServices* pSvc = nullptr;
    hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, 0, NULL, 0, 0, &pSvc);
    pLoc->Release();
    if (FAILED(hr)) {
        CoUninitialize();
        LogMessage(L"ConnectServer failed: " + std::to_wstring(hr), L"service_error.log");
        return hr;
    }

    hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
    if (FAILED(hr)) {
        pSvc->Release();
        CoUninitialize();
        LogMessage(L"CoSetProxyBlanket failed: " + std::to_wstring(hr), L"service_error.log");
        return hr;
    }

    IEnumWbemClassObject* pEnumerator = nullptr;
    std::wstring query = L"SELECT * FROM Win32_NetworkAdapter WHERE NetConnectionID = '" + adapterName + L"'";
    hr = pSvc->ExecQuery(_bstr_t(L"WQL"), _bstr_t(query.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);
    if (FAILED(hr)) {
        pSvc->Release();
        CoUninitialize();
        LogMessage(L"ExecQuery failed: " + std::to_wstring(hr), L"service_error.log");
        return hr;
    }

    IWbemClassObject* pclsObj = nullptr;
    ULONG uReturn = 0;
    std::wstring adapterGuid;
    while (pEnumerator) {
        hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn == 0) break;

        VARIANT vtProp;
        VariantInit(&vtProp);
        hr = pclsObj->Get(L"GUID", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
            adapterGuid = vtProp.bstrVal;
        }
        VariantClear(&vtProp);
        pclsObj->Release();
    }
    pEnumerator->Release();
    pSvc->Release();
    CoUninitialize();

    if (adapterGuid.empty()) {
        LogMessage(L"Could not find GUID for adapter: " + adapterName, L"service_error.log");
        return ERROR_NOT_FOUND;
    }

    std::wstring regPath = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\" + adapterGuid;
    HKEY hKey;
    DWORD disposition;
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hKey, &disposition) != ERROR_SUCCESS) {
        LogMessage(L"RegCreateKeyEx failed: " + std::to_wstring(GetLastError()), L"service_error.log");
        return GetLastError();
    }

    DWORD enableDhcp = 0;
    RegSetValueEx(hKey, L"EnableDHCP", 0, REG_DWORD, (BYTE*)&enableDhcp, sizeof(DWORD));
    RegSetValueEx(hKey, L"IPAddress", 0, REG_MULTI_SZ, (BYTE*)ipAddress.c_str(), SafeSizeToDword((ipAddress.length() + 2) * sizeof(wchar_t)));
    RegSetValueEx(hKey, L"SubnetMask", 0, REG_MULTI_SZ, (BYTE*)subnetMask.c_str(), SafeSizeToDword((subnetMask.length() + 2) * sizeof(wchar_t)));

    std::wstring gatewayValue = gateway.empty() ? L"\0" : gateway + L"\0";
    RegSetValueEx(hKey, L"DefaultGateway", 0, REG_MULTI_SZ, (BYTE*)gatewayValue.c_str(), SafeSizeToDword((gatewayValue.length() + 1) * sizeof(wchar_t)));

    std::wstring dnsValue = dns.empty() ? L"" : dns;
    RegSetValueEx(hKey, L"NameServer", 0, REG_SZ, (BYTE*)dnsValue.c_str(), SafeSizeToDword((dnsValue.length() + 1) * sizeof(wchar_t)));

    RegCloseKey(hKey);
    return ERROR_SUCCESS;
}

DWORD SetAdapterToDhcpViaRegistry(const std::wstring& adapterName) {
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        LogMessage(L"CoInitialize failed: " + std::to_wstring(hr), L"service_error.log");
        return hr;
    }

    IWbemLocator* pLoc = nullptr;
    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hr)) {
        CoUninitialize();
        LogMessage(L"CoCreateInstance failed: " + std::to_wstring(hr), L"service_error.log");
        return hr;
    }

    IWbemServices* pSvc = nullptr;
    hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, 0, NULL, 0, 0, &pSvc);
    pLoc->Release();
    if (FAILED(hr)) {
        CoUninitialize();
        LogMessage(L"ConnectServer failed: " + std::to_wstring(hr), L"service_error.log");
        return hr;
    }

    hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
    if (FAILED(hr)) {
        pSvc->Release();
        CoUninitialize();
        LogMessage(L"CoSetProxyBlanket failed: " + std::to_wstring(hr), L"service_error.log");
        return hr;
    }

    IEnumWbemClassObject* pEnumerator = nullptr;
    std::wstring query = L"SELECT * FROM Win32_NetworkAdapter WHERE NetConnectionID = '" + adapterName + L"'";
    hr = pSvc->ExecQuery(_bstr_t(L"WQL"), _bstr_t(query.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);
    if (FAILED(hr)) {
        pSvc->Release();
        CoUninitialize();
        LogMessage(L"ExecQuery failed: " + std::to_wstring(hr), L"service_error.log");
        return hr;
    }

    IWbemClassObject* pclsObj = nullptr;
    ULONG uReturn = 0;
    std::wstring adapterGuid;
    while (pEnumerator) {
        hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn == 0) break;

        VARIANT vtProp;
        VariantInit(&vtProp);
        hr = pclsObj->Get(L"GUID", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
            adapterGuid = vtProp.bstrVal;
        }
        VariantClear(&vtProp);
        pclsObj->Release();
    }
    pEnumerator->Release();
    pSvc->Release();
    CoUninitialize();

    if (adapterGuid.empty()) {
        LogMessage(L"Could not find GUID for adapter: " + adapterName, L"service_error.log");
        return ERROR_NOT_FOUND;
    }

    std::wstring regPath = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\" + adapterGuid;
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) {
        LogMessage(L"RegOpenKeyEx failed: " + std::to_wstring(GetLastError()), L"service_error.log");
        return GetLastError();
    }

    DWORD enableDhcp = 1;
    RegSetValueEx(hKey, L"EnableDHCP", 0, REG_DWORD, (BYTE*)&enableDhcp, sizeof(DWORD));
    const wchar_t* ipAddress = L"0.0.0.0\0\0";
    RegSetValueEx(hKey, L"IPAddress", 0, REG_MULTI_SZ, (BYTE*)ipAddress, SafeSizeToDword((wcslen(ipAddress) + 2) * sizeof(wchar_t)));
    const wchar_t* subnetMask = L"0.0.0.0\0\0";
    RegSetValueEx(hKey, L"SubnetMask", 0, REG_MULTI_SZ, (BYTE*)subnetMask, SafeSizeToDword((wcslen(subnetMask) + 2) * sizeof(wchar_t)));
    const wchar_t* defaultGateway = L"\0\0";
    RegSetValueEx(hKey, L"DefaultGateway", 0, REG_MULTI_SZ, (BYTE*)defaultGateway, 2 * sizeof(wchar_t));
    RegSetValueEx(hKey, L"NameServer", 0, REG_SZ, (BYTE*)L"", sizeof(wchar_t));

    RegCloseKey(hKey);
    return ERROR_SUCCESS;


}

DWORD SetStaticIP(const std::wstring& presetData) {
    std::vector<std::wstring> params;
    std::wstringstream ss(presetData);
    std::wstring item;
    while (std::getline(ss, item, L'|')) {
        params.push_back(item);
    }

    if (params.size() < 3) {
        LogMessage(L"Invalid preset data: " + presetData, L"service_error.log");
        return ERROR_INVALID_PARAMETER;
    }

    std::wstring adapterName = params[0];
    std::wstring ipAddress = params[1];
    std::wstring subnetMask = params[2];
    std::wstring gateway = params.size() > 3 ? params[3] : L"";
    std::wstring dns = params.size() > 4 ? params[4] : L"";

    bool isAdapterActive = IsAdapterActive(adapterName);
    DWORD result;

    if (isAdapterActive) {
        std::wstring command = L"interface ipv4 set address name=\"" + adapterName + L"\" source=static address=" + ipAddress + L" mask=" + subnetMask;
        if (!gateway.empty()) {
            command += L" gateway=" + gateway + L" gwmetric=1";
        }
        else {
            command += L" gateway=none";
        }

        result = RunNetshCommand(command, L"netsh_output_static.log");
        if (result != ERROR_SUCCESS) {
            LogMessage(L"netsh command failed for SetStaticIP", L"service_error.log");
            return result;
        }

        if (!dns.empty()) {
            command = L"interface ipv4 set dns name=\"" + adapterName + L"\" source=static address=" + dns;
            result = RunNetshCommand(command, L"netsh_output_static.log");
            if (result != ERROR_SUCCESS) {
                LogMessage(L"netsh command failed for DNS in SetStaticIP", L"service_error.log");
                return result;
            }
        }
    }
    else {
        result = SetAdapterToStaticIpViaRegistry(adapterName, ipAddress, subnetMask, gateway, dns);
        if (result != ERROR_SUCCESS) {
            LogMessage(L"SetAdapterToStaticIpViaRegistry failed", L"service_error.log");
            return result;
        }
        ResetAdapter(adapterName);
    }

    return ERROR_SUCCESS;
}

DWORD ResetToDhcp(const std::wstring& adapterName) {
    bool isAdapterActive = IsAdapterActive(adapterName);
    DWORD result;

    if (isAdapterActive) {
        std::wstring command = L"interface ipv4 set address name=\"" + adapterName + L"\" source=dhcp";
        result = RunNetshCommand(command, L"netsh_output_dhcp.log");
        if (result != ERROR_SUCCESS) {
            LogMessage(L"netsh command failed for ResetToDhcp", L"service_error.log");
            return result;
        }

        command = L"interface ipv4 set dns name=\"" + adapterName + L"\" source=dhcp";
        result = RunNetshCommand(command, L"netsh_output_dhcp.log");
        if (result != ERROR_SUCCESS) {
            LogMessage(L"netsh command failed for DNS in ResetToDhcp", L"service_error.log");
            return result;
        }
    }
    else {
        result = SetAdapterToDhcpViaRegistry(adapterName);
        if (result != ERROR_SUCCESS) {
            LogMessage(L"SetAdapterToDhcpViaRegistry failed", L"service_error.log");
            return result;
        }
        ResetAdapter(adapterName);
    }

    return ERROR_SUCCESS;
}
