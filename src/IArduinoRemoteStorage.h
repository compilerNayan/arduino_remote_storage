#ifndef IARDUINO_REMOTE_STORAGE_H
#define IARDUINO_REMOTE_STORAGE_H

#include <StandardDefines.h>

DefineStandardPointers(IArduinoRemoteStorage)

class IArduinoRemoteStorage {
    Public Virtual ~IArduinoRemoteStorage() = default;

    /** @return Next command to execute, or empty string if none. */
    Public Virtual StdString GetCommand() = 0;

    /** Store a log line (timestampMs e.g. millis()). No-op in stub implementation. */
    Public Virtual Void StoreLog(ULong timestampMs, const StdString& message) = 0;
};

#endif /* IARDUINO_REMOTE_STORAGE_H */
