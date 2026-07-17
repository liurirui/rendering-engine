#include "Logger.h"

#include <chrono>
#include <ctime>
#include <iostream>
#include <sstream>
#include <cstdio>

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#else
#include <sys/stat.h>
#endif

NAMESPACE_START

std::mutex Logger::mutex_;
std::ofstream Logger::file_;
bool Logger::initialized_ = false;
bool Logger::consoleOpened_ = false;
std::string Logger::logFilePath_;

void Logger::Initialize(const std::string& logDirectory, bool openConsole) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (initialized_) {
            return;
        }

        if (openConsole) {
            OpenConsoleIfNeeded();
        }

        EnsureDirectory(logDirectory);
        logFilePath_ = logDirectory + "/engine.log";
        file_.open(logFilePath_, std::ios::out | std::ios::trunc);
        initialized_ = true;
    }

    Log(LogLevel::Info, "Logger initialized. Log file: " + logFilePath_);
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
    initialized_ = false;
}

void Logger::Info(const std::string& message) {
    Log(LogLevel::Info, message);
}

void Logger::Warn(const std::string& message) {
    Log(LogLevel::Warning, message);
}

void Logger::Error(const std::string& message) {
    Log(LogLevel::Error, message);
}

void Logger::Log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string line = "[" + CurrentTimeString() + "][" + LevelToString(level) + "] " + message;

    if (level == LogLevel::Error) {
        std::cerr << line << std::endl;
    }
    else {
        std::cout << line << std::endl;
    }

    if (file_.is_open()) {
        file_ << line << std::endl;
        file_.flush();
    }
}

bool Logger::IsInitialized() {
    return initialized_;
}

const std::string& Logger::GetLogFilePath() {
    return logFilePath_;
}

const char* Logger::LevelToString(LogLevel level) {
    switch (level) {
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

std::string Logger::CurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
    return buffer;
}

void Logger::EnsureDirectory(const std::string& directory) {
    if (directory.empty()) {
        return;
    }

#ifdef _WIN32
    CreateDirectoryA(directory.c_str(), nullptr);
#else
    mkdir(directory.c_str(), 0755);
#endif
}

void Logger::OpenConsoleIfNeeded() {
#ifdef _WIN32
    if (consoleOpened_) {
        return;
    }

    if (GetConsoleWindow() == nullptr) {
        AllocConsole();
        SetConsoleTitleA("RenderEngine Log Console");
    }

#ifdef _MSC_VER
    FILE* stream = nullptr;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    freopen_s(&stream, "CONIN$", "r", stdin);
#else
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);
#endif

    consoleOpened_ = true;
#endif
}

NAMESPACE_END
