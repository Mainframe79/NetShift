#include "NetShiftService.h"
#include "NamedPipeServer.h"
#include "Logger.h"
#include <format>

SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hServiceStatus;

volatile bool running = true;

PSECURITY_DESCRIPTOR pSD = nullptr;
HANDLE hPipe = INVALID_HANDLE_VALUE;
HANDLE hThread = nullptr;

void Cleanup() {
    LogMessage(L"Service is shutting down and cleaning up resources.", L"service_log.log");

    if (pSD != nullptr) {
        LocalFree(pSD);
        pSD = nullptr;
    }

    if (hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }

    if (hThread != nullptr) {
        CloseHandle(hThread);
        hThread = nullptr;
    }
}

void WINAPI ServiceControlHandler(DWORD controlCode) {
    switch (controlCode) {
    case SERVICE_CONTROL_STOP:
        running = false;
        Cleanup();
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
    ServiceStatus = { SERVICE_WIN32_OWN_PROCESS, SERVICE_START_PENDING, SERVICE_ACCEPT_STOP, 0, 0, 0, 0 };

    hServiceStatus = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceControlHandler);
    if (hServiceStatus == nullptr) {
        ServiceStatus.dwWin32ExitCode = GetLastError();
        LogMessage(std::format(L"RegisterServiceCtrlHandler failed: {}", ServiceStatus.dwWin32ExitCode), L"service_error.log");
        return;
    }

    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hServiceStatus, &ServiceStatus);

    hThread = CreateThread(nullptr, 0, PipeServerThread, nullptr, 0, nullptr);
    if (hThread == nullptr) {
        ServiceStatus.dwWin32ExitCode = GetLastError();
        LogMessage(std::format(L"CreateThread failed: {}", ServiceStatus.dwWin32ExitCode), L"service_error.log");
        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(hServiceStatus, &ServiceStatus);
        return;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
}

int wmain(int argc, wchar_t* argv[]) {
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        { (LPWSTR)SERVICE_NAME, ServiceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcher(ServiceTable)) {
        LogMessage(std::format(L"StartServiceCtrlDispatcher failed: {}", GetLastError()), L"service_error.log");
        return GetLastError();
    }

    return 0;
}