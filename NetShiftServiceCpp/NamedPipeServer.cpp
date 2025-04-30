#include "NamedPipeServer.h"
#include "IPManager.h"
#include "Logger.h"
#include <string>
#include <sddl.h>

DWORD WINAPI PipeServerThread(LPVOID lpParam) {
    HANDLE hPipe;
    wchar_t buffer[1024];
    DWORD bytesRead;

    // Create a security descriptor that allows access only to administrators
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;

    // Define a security descriptor in SDDL format
    // "D:(A;;GA;;;BA)" means: Deny all access by default, Allow Generic All (GA) to Built-in Administrators (BA)
    PSECURITY_DESCRIPTOR pSD = NULL;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
        L"D:(A;;GA;;;AU)", // Allow full access to Authenticated Users
        SDDL_REVISION_1,
        &pSD,
        NULL))
    {
        LogMessage(L"ConvertStringSecurityDescriptorToSecurityDescriptorW failed: " + std::to_wstring(GetLastError()), L"service_error.log");
        return 1;
    }

    sa.lpSecurityDescriptor = pSD;

    while (true) {
        hPipe = CreateNamedPipe(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            1024,
            1024,
            0,
            &sa); // Pass the security attributes

        if (hPipe == INVALID_HANDLE_VALUE) {
            LogMessage(L"CreateNamedPipe failed: " + std::to_wstring(GetLastError()), L"service_error.log");
            LocalFree(pSD);
            return 1;
        }

        if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            while (ReadFile(hPipe, buffer, sizeof(buffer) - sizeof(wchar_t), &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead / sizeof(wchar_t)] = L'\0';
                std::wstring message(buffer);

                size_t pos = message.find(L"|");
                if (pos == std::wstring::npos) continue;

                std::wstring command = message.substr(0, pos);
                std::wstring params = message.substr(pos + 1);

                if (command == L"SetStaticIP") {
                    SetStaticIP(params);
                }
                else if (command == L"ResetToDhcp") {
                    ResetToDhcp(params);
                }
            }
        }
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }

    LocalFree(pSD);
    return 0;
}