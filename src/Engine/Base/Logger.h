#pragma once

#include "Constants.h"
#include <fstream>
#include <mutex>
#include <string>

NAMESPACE_START

enum class LogLevel {
    Info,
    Warning,
    Error
};

class Logger {
public:
    static void Initialize(const std::string& logDirectory = "logs", bool openConsole = true);
    static void Shutdown();

    static void Info(const std::string& message);
    static void Warn(const std::string& message);
    static void Error(const std::string& message);
    static void Log(LogLevel level, const std::string& message);

    static bool IsInitialized();
    static const std::string& GetLogFilePath();

private:
    static const char* LevelToString(LogLevel level);
    static std::string CurrentTimeString();
    static void EnsureDirectory(const std::string& directory);
    static void OpenConsoleIfNeeded();

    static std::mutex mutex_;
    static std::ofstream file_;
    static bool initialized_;
    static bool consoleOpened_;
    static std::string logFilePath_;
};

NAMESPACE_END
