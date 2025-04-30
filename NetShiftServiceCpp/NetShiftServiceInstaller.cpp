#include <windows.h>
#include <string>
#include <iostream>

#define SERVICE_NAME L"NetShiftService"
#define DISPLAY_NAME L"NetShift Service"
#define SERVICE_DESCRIPTION L"Service to handle IP address changes for the NetShift application."

// Function to install the service
bool InstallService(const std::wstring& serviceBinaryPath) {
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL) {
        std::wcerr << L"OpenSCManager failed: " << GetLastError() << std::endl;
        return false;
    }

    // Create the service
    SC_HANDLE schService = CreateService(
        schSCManager,               // SCM manager
        SERVICE_NAME,               // Service name
        DISPLAY_NAME,               // Display name
        SERVICE_ALL_ACCESS,         // Desired access
        SERVICE_WIN32_OWN_PROCESS,  // Service type
        SERVICE_AUTO_START,         // Start type (automatic)
        SERVICE_ERROR_NORMAL,       // Error control
        serviceBinaryPath.c_str(),  // Path to the service binary
        NULL,                       // No load ordering group
        NULL,                       // No tag identifier
        NULL,                       // No dependencies
        NULL,                       // Local System account
        NULL                        // No password (for Local System)
    );

    if (schService == NULL) {
        std::wcerr << L"CreateService failed: " << GetLastError() << std::endl;
        CloseServiceHandle(schSCManager);
        return false;
    }

    // Set the service description
    SERVICE_DESCRIPTIONW description = { 0 };
    description.lpDescription = (LPWSTR)SERVICE_DESCRIPTION;
    if (!ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &description)) {
        std::wcerr << L"ChangeServiceConfig2 (description) failed: " << GetLastError() << std::endl;
        // Not fatal, continue
    }

    std::wcout << L"Service installed successfully." << std::endl;

    // Start the service
    if (!StartService(schService, 0, NULL)) {
        std::wcerr << L"StartService failed: " << GetLastError() << std::endl;
        // Not fatal, user can start it manually
    }
    else {
        std::wcout << L"Service started successfully." << std::endl;
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return true;
}

// Function to uninstall the service
bool UninstallService() {
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL) {
        std::wcerr << L"OpenSCManager failed: " << GetLastError() << std::endl;
        return false;
    }

    SC_HANDLE schService = OpenService(schSCManager, SERVICE_NAME, SERVICE_ALL_ACCESS);
    if (schService == NULL) {
        std::wcerr << L"OpenService failed: " << GetLastError() << std::endl;
        CloseServiceHandle(schSCManager);
        return false;
    }

    // Stop the service if it's running
    SERVICE_STATUS status;
    if (ControlService(schService, SERVICE_CONTROL_STOP, &status)) {
        std::wcout << L"Service stopped successfully." << std::endl;
        // Wait for the service to stop
        while (QueryServiceStatus(schService, &status) && status.dwCurrentState != SERVICE_STOPPED) {
            Sleep(1000);
        }
    }

    // Delete the service
    if (!DeleteService(schService)) {
        std::wcerr << L"DeleteService failed: " << GetLastError() << std::endl;
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
        // Ensure the binary path is properly quoted if it contains spaces
        if (binaryPath.find(L" ") != std::wstring::npos && binaryPath[0] != L'"') {
            binaryPath = L"\"" + binaryPath + L"\"";
        }

        if (!InstallService(binaryPath)) {
            return 1;
        }
    }
    else if (command == L"--uninstall") {
        if (!UninstallService()) {
            return 1;
        }
    }
    else {
        std::wcerr << L"Invalid command. Use --install or --uninstall." << std::endl;
        return 1;
    }

    return 0;
}