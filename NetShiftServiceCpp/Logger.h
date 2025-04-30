#ifndef LOGGER_H
#define LOGGER_H

#include <string>

// Log directory path
const std::wstring LOG_DIR = std::wstring(L"C:\\ProgramData\\ZentrixLabs\\NetShift\\Logs");

// Function declaration for logging
void LogMessage(const std::wstring& message, const std::wstring& fileName);

#endif // LOGGER_H