#include "Logger.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <ctime>

void LogMessage(const std::wstring& message, const std::wstring& fileName) {
    CreateDirectory(LOG_DIR.c_str(), NULL);

    std::wofstream logFile(LOG_DIR + L"\\" + fileName, std::ios::app);
    if (!logFile.is_open())
        return;

    // Add timestamp
    std::wstringstream timestamped;
    std::time_t now = std::time(nullptr);
    struct tm localTime;
    localtime_s(&localTime, &now);

    wchar_t buffer[64];
    wcsftime(buffer, sizeof(buffer) / sizeof(wchar_t), L"[%Y-%m-%d %H:%M:%S] ", &localTime);
    timestamped << buffer << message;

    logFile << timestamped.str() << L"\n";
    logFile.close();
}
