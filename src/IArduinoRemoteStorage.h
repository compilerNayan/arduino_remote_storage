#ifndef IARDUINO_REMOTE_STORAGE_H
#define IARDUINO_REMOTE_STORAGE_H

#include <StandardDefines.h>
#include "IFirebaseOperations.h"

DefineStandardPointers(IArduinoRemoteStorage)

class IArduinoRemoteStorage {
    Public Virtual ~IArduinoRemoteStorage() = default;

    /** Fills @param out with the next command to execute. Returns operation result. */
    Public Virtual FirebaseOperationResult GetCommand(StdString& out) = 0;

    /** Publish logs. Returns operation result. */
    Public Virtual FirebaseOperationResult PublishLogs() = 0;
};

#endif /* IARDUINO_REMOTE_STORAGE_H */
