#ifndef __LOG_H__
#define __LOG_H__

#include <cstdio>
#include <cstdarg>
#include <string>
#include <vector>

#define LOG_CONSOLE(format, ...) LogConsole(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) LogDebug(__FILE__, __LINE__, format, ##__VA_ARGS__)

void LogConsole(const char file[], int line, const char* format, ...);
void LogDebug(const char file[], int line, const char* format, ...);

class ConsoleLog
{
public:
    static ConsoleLog& GetInstance();

    void AddLog(const std::string& message);
    void Clear();
    const std::vector<std::string>& GetLogs() const { return logs; }

    void Shutdown()
    {
        logs.clear();
        logs.shrink_to_fit();
    }

private:
    ConsoleLog() = default;
    ~ConsoleLog() = default;

    ConsoleLog(const ConsoleLog&) = delete;
    ConsoleLog& operator=(const ConsoleLog&) = delete;

    std::vector<std::string> logs;
};

#endif  // __LOG_H__

