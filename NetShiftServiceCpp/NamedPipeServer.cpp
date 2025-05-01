#include "NamedPipeServer.h"
#include "IPManager.h"
#include "Logger.h"
#include <string>
#include <sddl.h>

extern PSECURITY_DESCRIPTOR pSD;
extern HANDLE hPipe;

bool running = true;

DWORD WINAPI PipeServerThread(LPVOID lpParam) {
    wchar_t buffer[1024];
    DWORD bytesRead;

    // Create a security descriptor that allows access to authenticated users
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;

    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
        L"D:(A;;GA;;;AU)", // Allow full access to Authenticated Users
        SDDL_REVISION_1,
        &pSD,
        nullptr))
    {
        LogMessage(L"ConvertStringSecurityDescriptorToSecurityDescriptorW failed: " + std::to_wstring(GetLastError()), L"service_error.log");
        return 1;
    }

    sa.lpSecurityDescriptor = pSD;

    while (running) {
        hPipe = CreateNamedPipe(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            1024,
            1024,
            0,
            &sa);

        if (hPipe == INVALID_HANDLE_VALUE) {
            LogMessage(L"CreateNamedPipe failed: " + std::to_wstring(GetLastError()), L"service_error.log");
            LocalFree(pSD);
            return 1;
        }

        if (ConnectNamedPipe(hPipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
            while (ReadFile(hPipe, buffer, sizeof(buffer) - sizeof(wchar_t), &bytesRead, nullptr) && bytesRead > 0) {
                buffer[bytesRead / sizeof(wchar_t)] = L'\0';
                std::wstring message(buffer);

                size_t pos = message.find(L"|");
                if (pos == std::wstring::npos) {
                    LogMessage(L"Invalid message format: " + message, L"service_error.log");
                    continue;
                }

                std::wstring command = message.substr(0, pos);
                std::wstring params = message.substr(pos + 1);

                std::wstring response = L"Success";
                if (command == L"SetStaticIP") {
                    DWORD result = SetStaticIP(params);
                    if (result != ERROR_SUCCESS) {
                        response = L"Error: " + std::to_wstring(result);
                    }
                }
                else if (command == L"ResetToDhcp") {
                    DWORD result = ResetToDhcp(params);
                    if (result != ERROR_SUCCESS) {
                        response = L"Error: " + std::to_wstring(result);
                    }
                }
                else if (command == L"Shutdown") {
                    running = false;
                    response = L"Server shutting down";
                }
                else {
                    response = L"Unknown command";
                }

                WriteFile(hPipe, response.c_str(), (response.length() + 1) * sizeof(wchar_t), nullptr, nullptr);
            }
        }
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }

    LocalFree(pSD);
    return 0;
}
