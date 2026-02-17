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

    /** Returns true if RetrieveCommands or PublishLogs is currently running. */
    Public Virtual Bool IsOperationInProgress() const = 0;

    /** Returns true if the instance is dirty (e.g. after an error); public methods will return default/empty until reset. */
    Public Virtual Bool IsDirty() const = 0;
};

#endif /* IFIREBASEOPERATIONS_H */
