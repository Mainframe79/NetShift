#include "Logger.h"
#include <windows.h>
#include <fstream>
#include <filesystem>
#include <format>

namespace fs = std::filesystem;

void LogMessage(const std::wstring& message, const std::wstring& fileName) {
    // Ensure log directory exists
    fs::create_directories(LOG_DIR);

    // Construct full log file path
    std::wstring logPath = LOG_DIR + L"\\" + fileName;

    // Open file for appending
    std::wofstream logFile(logPath, std::ios::app);
    if (!logFile.is_open()) return;

    // Get current time
    SYSTEMTIME st;
    GetLocalTime(&st);

    // Format timestamp
    std::wstring timestamp = std::format(
        L"[{:04}-{:02}-{:02} {:02}:{:02}:{:02}] ",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond
    );

    // Write log entry
    logFile << timestamp << message;
    if (!message.empty() && message.back() != L'\n') {
        logFile << L"\n";
    }
}
