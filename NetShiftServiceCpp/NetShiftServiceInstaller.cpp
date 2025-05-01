#include <windows.h>
#include <string>
#include <iostream>
#include <format>

#define SERVICE_NAME L"NetShiftService"
#define DISPLAY_NAME L"NetShift Service"
#define SERVICE_DESCRIPTION L"Service to handle IP address changes for the NetShift application."

bool InstallService(const std::wstring& serviceBinaryPath) {
    SC_HANDLE schSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!schSCManager) {
        std::wcerr << std::format(L"OpenSCManager failed: {}", GetLastError()) << std::endl;
        return false;
    }

    SC_HANDLE schService = CreateService(
        schSCManager,
        SERVICE_NAME,
        DISPLAY_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        serviceBinaryPath.c_str(),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    if (!schService) {
        std::wcerr << std::format(L"CreateService failed: {}", GetLastError()) << std::endl;
        CloseServiceHandle(schSCManager);
        return false;
    }

    SERVICE_DESCRIPTIONW description = { const_cast<LPWSTR>(SERVICE_DESCRIPTION) };
    if (!ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &description)) {
        std::wcerr << std::format(L"ChangeServiceConfig2 (description) failed: {}", GetLastError()) << std::endl;
    }

    std::wcout << L"Service installed successfully." << std::endl;

    if (!StartService(schService, 0, nullptr)) {
        std::wcerr << std::format(L"StartService failed: {}", GetLastError()) << std::endl;
    }
    else {
        std::wcout << L"Service started successfully." << std::endl;
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return true;
}

bool UninstallService() {
    SC_HANDLE schSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!schSCManager) {
        std::wcerr << std::format(L"OpenSCManager failed: {}", GetLastError()) << std::endl;
        return false;
    }

    SC_HANDLE schService = OpenService(schSCManager, SERVICE_NAME, SERVICE_ALL_ACCESS);
    if (!schService) {
        std::wcerr << std::format(L"OpenService failed: {}", GetLastError()) << std::endl;
        CloseServiceHandle(schSCManager);
        return false;
    }

    SERVICE_STATUS status;
    if (ControlService(schService, SERVICE_CONTROL_STOP, &status)) {
        std::wcout << L"Service stopped successfully." << std::endl;
        while (QueryServiceStatus(schService, &status) && status.dwCurrentState != SERVICE_STOPPED) {
            Sleep(1000);
        }
    }

    if (!DeleteService(schService)) {
        std::wcerr << std::format(L"DeleteService failed: {}", GetLastError()) << std::endl;
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return false;
    }

    std::wcout << L"Service uninstalled successfully." << std::endl;

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return true;
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 2) {
        std::wcerr << L"Usage: NetShiftServiceInstaller.exe [--install <path_to_binary> | --uninstall]" << std::endl;
        return 1;
    }

    std::wstring command = argv[1];

    if (command == L"--install") {
        if (argc != 3) {
            std::wcerr << L"Usage for install: NetShiftServiceInstaller.exe --install <path_to_binary>" << std::endl;
            return 1;
        }

        std::wstring binaryPath = argv[2];
        if (binaryPath.find(L" ") != std::wstring::npos && binaryPath.front() != L'"') {
            binaryPath = std::format(L"\"{}\"", binaryPath);
        }

        if (!InstallService(binaryPath)) return 1;

    }
    else if (command == L"--uninstall") {
        if (!UninstallService()) return 1;

    }
    else {
        std::wcerr << L"Invalid command. Use --install or --uninstall." << std::endl;
        return 1;
    }

    return 0;
}