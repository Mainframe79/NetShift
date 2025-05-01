#include "IPManager.h"
#include "Logger.h"
#include <windows.h>
#include <vector>
#include <iphlpapi.h>
#include <comdef.h>
#include <wbemidl.h>
#include <sstream>
#include <format>
#include <string>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wbemuuid.lib")

DWORD SafeSizeToDword(size_t size) {
    if (size > MAXDWORD) {
        throw std::overflow_error("Size exceeds maximum DWORD value");
    }
    return static_cast<DWORD>(size);
}

bool LogIfFailed(HRESULT hr, const std::wstring& context, const std::wstring& file) {
    if (FAILED(hr)) {
        LogMessage(std::format(L"{} failed: HRESULT=0x{:08X}", context, static_cast<unsigned int>(hr)), file);
        return true;
    }
    return false;
}

DWORD RunNetshCommand(const std::wstring& command, const std::wstring& logFile) {
    SECURITY_ATTRIBUTES sa{ sizeof(sa), nullptr, TRUE };
    HANDLE hStdOutRead, hStdOutWrite;
    CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0);
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{ sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;

    PROCESS_INFORMATION pi{};
    std::wstring cmd = std::format(L"netsh {}", command);

    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        LogMessage(std::format(L"CreateProcess failed: {}", GetLastError()), logFile);
        return GetLastError();
    }

    CloseHandle(hStdOutWrite);
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    wchar_t buffer[1024];
    DWORD bytesRead;
    std::wstring output;
    while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - sizeof(wchar_t), &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead / sizeof(wchar_t)] = L'\0';
        output += buffer;
    }

    LogMessage(std::format(L"Command: {}\nOutput: {}", cmd, output), logFile);

    CloseHandle(hStdOutRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode;
}

void ResetAdapter(const std::wstring& adapterName) {
    RunNetshCommand(std::format(L"interface set interface name=\"{}\" admin=disable", adapterName), L"netsh_reset.log");
    RunNetshCommand(std::format(L"interface set interface name=\"{}\" admin=enable", adapterName), L"netsh_reset.log");
}

bool IsAdapterActive(const std::wstring& adapterName) {
    ULONG bufferSize = 0;
    GetIfTable(nullptr, &bufferSize, FALSE);
    std::vector<BYTE> buffer(bufferSize);
    auto* pIfTable = reinterpret_cast<PMIB_IFTABLE>(buffer.data());

    if (GetIfTable(pIfTable, &bufferSize, FALSE) == NO_ERROR) {
        for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
            std::wstring name(pIfTable->table[i].wszName);
            if (name == adapterName && pIfTable->table[i].dwOperStatus == IF_OPER_STATUS_OPERATIONAL) {
                return true;
            }
        }
    }
    return false;
}

DWORD SetStaticIP(const std::wstring& presetData) {
    std::wstringstream ss(presetData);
    std::vector<std::wstring> parts;
    std::wstring segment;
    while (std::getline(ss, segment, L'|')) {
        parts.push_back(segment);
    }

    if (parts.size() < 3) {
        LogMessage(L"SetStaticIP: Invalid preset format.", L"service_error.log");
        return ERROR_INVALID_PARAMETER;
    }

    const std::wstring& adapter = parts[0];
    const std::wstring& ip = parts[1];
    const std::wstring& mask = parts[2];
    const std::wstring gateway = parts.size() > 3 ? parts[3] : L"";
    const std::wstring dns = parts.size() > 4 ? parts[4] : L"";

    std::wstring cmd = std::format(L"interface ipv4 set address name=\"{}\" source=static address={} mask={} {}",
        adapter, ip, mask,
        gateway.empty() ? L"gateway=none" : std::format(L"gateway={} gwmetric=1", gateway));

    DWORD result = RunNetshCommand(cmd, L"netsh_output_static.log");
    if (result != ERROR_SUCCESS) return result;

    if (!dns.empty()) {
        cmd = std::format(L"interface ipv4 set dns name=\"{}\" source=static address={}", adapter, dns);
        result = RunNetshCommand(cmd, L"netsh_output_static.log");
        if (result != ERROR_SUCCESS) return result;
    }

    return ERROR_SUCCESS;
}

DWORD ResetToDhcp(const std::wstring& adapterName) {
    std::wstring cmd = std::format(L"interface ipv4 set address name=\"{}\" source=dhcp", adapterName);
    DWORD result = RunNetshCommand(cmd, L"netsh_output_dhcp.log");
    if (result != ERROR_SUCCESS) return result;

    cmd = std::format(L"interface ipv4 set dns name=\"{}\" source=dhcp", adapterName);
    return RunNetshCommand(cmd, L"netsh_output_dhcp.log");
}
