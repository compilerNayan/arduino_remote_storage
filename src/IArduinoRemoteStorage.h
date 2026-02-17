#ifndef IARDUINO_REMOTE_STORAGE_H
#define IARDUINO_REMOTE_STORAGE_H

#include <StandardDefines.h>

DefineStandardPointers(IArduinoRemoteStorage)

class IArduinoRemoteStorage {
    Public Virtual ~IArduinoRemoteStorage() = default;

    /** @return Next command to execute, or empty string if none. */
    Public Virtual StdString GetCommand() = 0;

    /** Publish logs */
    Public Virtual Void PublishLogs() = 0;
};

#endif /* IARDUINO_REMOTE_STORAGE_H */
