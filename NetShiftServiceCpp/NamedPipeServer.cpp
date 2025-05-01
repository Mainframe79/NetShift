#include "NamedPipeServer.h"
#include "IPManager.h"
#include "Logger.h"
#include <string>
#include <sddl.h>
#include <format>

extern PSECURITY_DESCRIPTOR pSD;
extern HANDLE hPipe;

bool running = true;

std::wstring GetErrorMessage(DWORD errorCode) {
    wchar_t* msgBuffer = nullptr;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorCode, 0, (LPWSTR)&msgBuffer, 0, nullptr);

    std::wstring message = msgBuffer ? msgBuffer : L"Unknown error";
    if (msgBuffer)
        LocalFree(msgBuffer);

    return message;
}

DWORD WINAPI PipeServerThread(LPVOID lpParam) {
    wchar_t buffer[1024];
    DWORD bytesRead;

    SECURITY_ATTRIBUTES sa{ sizeof(SECURITY_ATTRIBUTES), nullptr, FALSE };

    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
        L"D:(A;;GA;;;AU)",
        SDDL_REVISION_1,
        &pSD,
        nullptr)) {
        LogMessage(std::format(L"ConvertStringSecurityDescriptorToSecurityDescriptorW failed: {}", GetLastError()), L"service_error.log");
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
            LogMessage(std::format(L"CreateNamedPipe failed: {}", GetLastError()), L"service_error.log");
            LocalFree(pSD);
            return 1;
        }

        if (ConnectNamedPipe(hPipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
            while (ReadFile(hPipe, buffer, sizeof(buffer) - sizeof(wchar_t), &bytesRead, nullptr) && bytesRead > 0) {
                buffer[bytesRead / sizeof(wchar_t)] = L'\0';
                std::wstring message(buffer);

                size_t pos = message.find(L"|");
                if (pos == std::wstring::npos) {
                    LogMessage(std::format(L"Invalid message format: {}", message), L"service_error.log");
                    continue;
                }

                std::wstring command = message.substr(0, pos);
                std::wstring params = message.substr(pos + 1);
                std::wstring response;

                if (command == L"SetStaticIP") {
                    DWORD result = SetStaticIP(params);
                    if (result == ERROR_SUCCESS) {
                        LogMessage(std::format(L"SetStaticIP successfully applied for parameters: {}", params), L"service_log.log");
                        response = L"SetStaticIP: Success";
                    }
                    else {
                        LogMessage(std::format(L"SetStaticIP failed with error: {} ({})", result, GetErrorMessage(result)), L"service_error.log");
                        response = std::format(L"SetStaticIP: Error {} ({})", result, GetErrorMessage(result));
                    }
                }
                else if (command == L"ResetToDhcp") {
                    DWORD result = ResetToDhcp(params);
                    if (result == ERROR_SUCCESS) {
                        LogMessage(std::format(L"ResetToDhcp successfully applied for adapter: {}", params), L"service_log.log");
                        response = L"ResetToDhcp: Success";
                    }
                    else {
                        LogMessage(std::format(L"ResetToDhcp failed with error: {} ({})", result, GetErrorMessage(result)), L"service_error.log");
                        response = std::format(L"ResetToDhcp: Error {} ({})", result, GetErrorMessage(result));
                    }
                }
                else if (command == L"Shutdown") {
                    running = false;
                    response = L"Server shutting down";
                }
                else {
                    response = L"Unknown command";
                }

                WriteFile(hPipe, response.c_str(), static_cast<DWORD>((response.length() + 1) * sizeof(wchar_t)), nullptr, nullptr);
            }
        }

        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }

    LocalFree(pSD);
    return 0;
}
