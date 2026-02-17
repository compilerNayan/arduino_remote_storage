#ifndef ILOGBUFFER_H
#define ILOGBUFFER_H

#include <StandardDefines.h>

DefineStandardPointers(ILogBuffer)

class ILogBuffer {
    Public Virtual ~ILogBuffer() = default;

    /** Add a log entry to the buffer (timestamp e.g. millis(), message). */
    Public Virtual Void AddLog(ULong timestampMs, const StdString& message) = 0;

    /** Add multiple log entries to the buffer in bulk. */
    Public Virtual Void AddLogs(const StdMap<ULong, StdString>& logs) = 0;

    /** Take all buffered logs and clear the buffer. Caller typically passes result to IFirebaseOperations::PublishLogs then discards. */
    Public Virtual StdMap<ULong, StdString> TakeLogs() = 0;
};

#endif /* ILOGBUFFER_H */
