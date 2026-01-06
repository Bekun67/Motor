#include "Log.h"
#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <string>

void LogDebug(const char file[], int line, const char* format, ...)
{
    static char tmpString1[4096];
    static va_list ap;

    // Construct the string from variable arguments
    va_start(ap, format);
    vsnprintf(tmpString1, 4096, format, ap);
    va_end(ap);

    // Construct the final log message
    std::string logMessage = std::string("\n") + file + "(" + std::to_string(line) + ") : " + tmpString1;

    // Print the formatted string to the standard error stream
    std::cerr << logMessage << std::endl;

}

void LogConsole(const char file[], int line, const char* format, ...)
{
    static char tmpString[4096];
    static va_list ap;

    va_start(ap, format);
    vsnprintf(tmpString, 4096, format, ap);
    va_end(ap);

    std::string message = tmpString;

    std::cerr << message << std::endl;

    ConsoleLog::GetInstance().AddLog(message);

}

ConsoleLog& ConsoleLog::GetInstance()
{
    static ConsoleLog instance;
    return instance;
}

void ConsoleLog::AddLog(const std::string& message)
{
    logs.push_back(message);

    if (logs.size() > 1000)
    {
        logs.erase(logs.begin());
    }
}

void ConsoleLog::Clear()
{
    logs.clear();
}