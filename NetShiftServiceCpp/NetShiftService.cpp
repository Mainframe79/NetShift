#include "NetShiftService.h"
#include "NamedPipeServer.h"
#include "Logger.h"

SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hServiceStatus;

volatile bool running = true;

// Global variables for cleanup
PSECURITY_DESCRIPTOR pSD = nullptr; // Security descriptor for the named pipe
HANDLE hPipe = INVALID_HANDLE_VALUE; // Handle for the named pipe
HANDLE hThread = nullptr; // Handle for the named pipe server thread

void Cleanup() {
    // Log cleanup
    LogMessage(L"Service is shutting down and cleaning up resources.", L"service_log.log");

    // Free the security descriptor
    if (pSD != nullptr) {
        LocalFree(pSD);
        pSD = nullptr;
    }

    // Close the named pipe handle
    if (hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }

    // Close the thread handle
    if (hThread != nullptr) {
        CloseHandle(hThread);
        hThread = nullptr;
    }
}

void WINAPI ServiceControlHandler(DWORD controlCode) {
    switch (controlCode) {
    case SERVICE_CONTROL_STOP:
        running = false; // Signal the named pipe server thread to stop
        Cleanup(); // Call cleanup function
        ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(hServiceStatus, &ServiceStatus);
        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(hServiceStatus, &ServiceStatus);
        break;
    default:
        break;
    }
}

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 0;

    hServiceStatus = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceControlHandler);
    if (hServiceStatus == nullptr) {
        ServiceStatus.dwWin32ExitCode = GetLastError();
        LogMessage(L"RegisterServiceCtrlHandler failed: " + std::to_wstring(ServiceStatus.dwWin32ExitCode), L"service_error.log");
        return;
    }

    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hServiceStatus, &ServiceStatus);

    hThread = CreateThread(nullptr, 0, PipeServerThread, nullptr, 0, nullptr);
    if (hThread == nullptr) {
        ServiceStatus.dwWin32ExitCode = GetLastError();
        LogMessage(L"CreateThread failed: " + std::to_wstring(ServiceStatus.dwWin32ExitCode), L"service_error.log");
        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(hServiceStatus, &ServiceStatus);
        return;
    }

    WaitForSingleObject(hThread, INFINITE); // Wait for the thread to exit
    CloseHandle(hThread);
}

int wmain(int argc, wchar_t* argv[]) {
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        { (LPWSTR)SERVICE_NAME, ServiceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcher(ServiceTable)) {
        LogMessage(L"StartServiceCtrlDispatcher failed: " + std::to_wstring(GetLastError()), L"service_error.log");
        return GetLastError();
    }

    return 0;
}
