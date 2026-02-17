#ifndef LOGBUFFER_H
#define LOGBUFFER_H

#include "ILogBuffer.h"
#include <mutex>

/* @Component */
class LogBuffer : public ILogBuffer {
    // ~70 bytes per record (8-byte key + ~60-char message from sample logs) → 100KB ≈ 1500, 20KB ≈ 300
    Private Static const Size kMaxRecordCount = 1500;   // ~100 KB
    Private Static const Size kTrimRecordCount = 300;   // ~20 KB

    Private StdMap<ULong, StdString> logs_;
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

    Public Void AddLog(ULong timestampMs, const StdString& message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        logs_[timestampMs] = message;
        TrimIfOverCapacity();
    }

    Public Void AddLogs(const StdMap<ULong, StdString>& logs) override {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pair : logs) {
            logs_[pair.first] = pair.second;
        }
        TrimIfOverCapacity();
    }

    Public StdMap<ULong, StdString> TakeLogs() override {
        std::lock_guard<std::mutex> lock(mutex_);
        StdMap<ULong, StdString> out;
        out.swap(logs_);
        return out;
    }
};

#endif /* LOGBUFFER_H */
