#ifndef ILOGBUFFER_H
#define ILOGBUFFER_H

#include <StandardDefines.h>

DefineStandardPointers(ILogBuffer)

class ILogBuffer {
    Public Virtual ~ILogBuffer() = default;

    /** Add a log entry to the buffer (key: unique timestamp+seq for ordering and Firebase key). */
    Public Virtual Void AddLog(ULongLong keyTimestampMs, const StdString& message) = 0;

    /** Add multiple log entries to the buffer in bulk. */
    Public Virtual Void AddLogs(const StdMap<ULongLong, StdString>& logs) = 0;

    /** Take all buffered logs and clear the buffer. Caller typically passes result to IFirebaseOperations::PublishLogs then discards. */
    Public Virtual StdMap<ULongLong, StdString> TakeLogs() = 0;
};

#endif /* ILOGBUFFER_H */
