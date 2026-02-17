#ifndef IFIREBASEOPERATIONS_H
#define IFIREBASEOPERATIONS_H

#include <StandardDefines.h>

DefineStandardPointers(IFirebaseOperations)

class IFirebaseOperations {
    Public Virtual ~IFirebaseOperations() = default;

    /** @return List of commands from Firebase (each element is "key:value"). */
    Public Virtual StdVector<StdString> RetrieveCommands() = 0;
};

#endif /* IFIREBASEOPERATIONS_H */
