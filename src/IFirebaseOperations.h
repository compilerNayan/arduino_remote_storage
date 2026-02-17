#ifndef IFIREBASEOPERATIONS_H
#define IFIREBASEOPERATIONS_H

#include <StandardDefines.h>

DefineStandardPointers(IFirebaseOperations)

class IFirebaseOperations {
    Public Virtual ~IFirebaseOperations() = default;

    /** @return List of commands from Firebase (each element is "key:value"). */
    Public Virtual StdVector<StdString> RetrieveCommands() = 0;

    /** Publish logs to Firebase at /logs. Map key = timestamp (e.g. millis()), value = message. Keys are written as ISO8601. */
    Public Virtual Bool PublishLogs(const StdMap<ULong, StdString>& logs) = 0;
};

#endif /* IFIREBASEOPERATIONS_H */
