#ifndef LOGBUFFER_H
#define LOGBUFFER_H

#include "ILogBuffer.h"
#include <mutex>

/* @Component */
class LogBuffer : public ILogBuffer {
    Private StdMap<ULong, StdString> logs_;
    Private std::mutex mutex_;

    Public LogBuffer() = default;

    Public Virtual ~LogBuffer() override = default;

    Public Void AddLog(ULong timestampMs, const StdString& message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        logs_[timestampMs] = message;
    }

    Public Void AddLogs(const StdMap<ULong, StdString>& logs) override {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pair : logs) {
            logs_[pair.first] = pair.second;
        }
    }

    Public StdMap<ULong, StdString> TakeLogs() override {
        std::lock_guard<std::mutex> lock(mutex_);
        StdMap<ULong, StdString> out;
        out.swap(logs_);
        return out;
    }
};

#endif /* LOGBUFFER_H */
