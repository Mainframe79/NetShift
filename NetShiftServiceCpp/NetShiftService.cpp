#include "NetShiftService.h"
#include "NamedPipeServer.h"
#include "Logger.h"

SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hServiceStatus;

void WINAPI ServiceControlHandler(DWORD controlCode) {
    switch (controlCode) {
    case SERVICE_CONTROL_STOP:
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
    if (hServiceStatus == NULL) {
        LogMessage(L"RegisterServiceCtrlHandler failed: " + std::to_wstring(GetLastError()), L"service_error.log");
        return;
    }

    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hServiceStatus, &ServiceStatus);

    // Start the named pipe server thread
    HANDLE hThread = CreateThread(NULL, 0, PipeServerThread, NULL, 0, NULL);
    if (hThread == NULL) {
        LogMessage(L"CreateThread failed: " + std::to_wstring(GetLastError()), L"service_error.log");
        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(hServiceStatus, &ServiceStatus);
        return;
    }

    CloseHandle(hThread);
}

int wmain(int argc, wchar_t* argv[]) {
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        { (LPWSTR)SERVICE_NAME, ServiceMain },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcher(ServiceTable)) {
        LogMessage(L"StartServiceCtrlDispatcher failed: " + std::to_wstring(GetLastError()), L"service_error.log");
        return GetLastError();
    }

    return 0;
}