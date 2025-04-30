#include "Logger.h"
#include <windows.h>
#include <fstream>

void LogMessage(const std::wstring& message, const std::wstring& fileName) {
    CreateDirectory(LOG_DIR.c_str(), NULL);
    std::wofstream logFile(LOG_DIR + L"\\" + fileName, std::ios::app);
    logFile << message << L"\n";
    logFile.close();
}