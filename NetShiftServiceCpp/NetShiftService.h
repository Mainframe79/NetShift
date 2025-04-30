#ifndef NETSHIFT_SERVICE_H
#define NETSHIFT_SERVICE_H

#include <windows.h>

#define SERVICE_NAME L"NetShiftService"
#define PIPE_NAME L"\\\\.\\pipe\\NetShiftService"

// Function declarations for the service
void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
void WINAPI ServiceControlHandler(DWORD controlCode);
int wmain(int argc, wchar_t* argv[]);

#endif // NETSHIFT_SERVICE_H