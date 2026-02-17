#ifndef ARDUINO_REMOTE_STORAGE_H
#define ARDUINO_REMOTE_STORAGE_H

#include "IArduinoRemoteStorage.h"

/* @Component */
class ArduinoRemoteStorage : public IArduinoRemoteStorage {
    Public StdString GetCommand() override { return StdString(); }

    Public Void StoreLog(const StdString&) override {}
};

#endif /* ARDUINO_REMOTE_STORAGE_H */
