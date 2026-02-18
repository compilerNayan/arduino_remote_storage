#ifndef LOGBUFFER_H
#define LOGBUFFER_H

#include "ILogBuffer.h"
#include <mutex>

/* @Component */
class LogBuffer : public ILogBuffer {
    // ~70 bytes per record (8-byte key + ~60-char message from sample logs) → 100KB ≈ 1500, 20KB ≈ 300
    Private Static const Size kMaxRecordCount = 1500;   // ~100 KB
    Private Static const Size kTrimRecordCount = 300;   // ~20 KB

    Private StdMap<ULongLong, StdString> logs_;
    Private std::mutex mutex_;

    /** If count exceeds kMaxRecordCount, remove oldest kTrimRecordCount entries. O(trim count). */
    Private Void TrimIfOverCapacity() {
        if (logs_.size() <= kMaxRecordCount) return;
        Size toRemove = kTrimRecordCount;
        for (auto it = logs_.begin(); toRemove > 0 && it != logs_.end(); ) {
            it = logs_.erase(it);
            --toRemove;
        }
    }

    Public LogBuffer() = default;

    Public Virtual ~LogBuffer() override = default;

    Public Void AddLog(ULongLong keyTimestampMs, const StdString& message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        logs_[keyTimestampMs] = message;
        TrimIfOverCapacity();
    }

    Public Void AddLogs(const StdMap<ULongLong, StdString>& logs) override {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pair : logs) {
            logs_[pair.first] = pair.second;
        }
        TrimIfOverCapacity();
    }

    Public StdMap<ULongLong, StdString> TakeLogs() override {
        std::lock_guard<std::mutex> lock(mutex_);
        StdMap<ULongLong, StdString> out;
        out.swap(logs_);
        return out;
    }

    Public StdMap<ULongLong, StdString> TakeLogsAtMost(Size maxCount) override {
        std::lock_guard<std::mutex> lock(mutex_);
        StdMap<ULongLong, StdString> out;
        Size n = 0;
        for (auto it = logs_.begin(); it != logs_.end() && n < maxCount; ) {
            out[it->first] = it->second;
            it = logs_.erase(it);
            ++n;
        }
        return out;
    }
};

#endif /* LOGBUFFER_H */
