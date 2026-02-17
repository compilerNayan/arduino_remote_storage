#ifdef ARDUINO
#ifndef ARDUINO_REMOTE_STORAGE_H
#define ARDUINO_REMOTE_STORAGE_H

#include "IArduinoRemoteStorage.h"
#include "IFirebaseOperations.h"
#include <Arduino.h>

#include <queue>
#include <mutex>

/* @Component */
class ArduinoRemoteStorage : public IArduinoRemoteStorage {
    /* @Autowired */
    Private IFirebaseOperationsPtr firebaseOperations;

    Private std::queue<StdString> requestQueue_;
    Private std::mutex requestQueueMutex_;

    Private Bool TryDequeue(StdString& out) {
        std::lock_guard<std::mutex> lock(requestQueueMutex_);
        if (requestQueue_.empty()) return false;
        out = requestQueue_.front();
        requestQueue_.pop();
        return true;
    }

    Private Void EnqueueAll(const StdVector<StdString>& commands) {
        std::lock_guard<std::mutex> lock(requestQueueMutex_);
        for (const StdString& s : commands) {
            requestQueue_.push(s);
        }
    }

    Public ArduinoRemoteStorage() = default;

    Public Virtual ~ArduinoRemoteStorage() override = default;

    Public StdString GetCommand() override {
        StdString out;
        if (TryDequeue(out)) {
            return out;
        }
        StdVector<StdString> commands = firebaseOperations->RetrieveCommands();
        EnqueueAll(commands);
        if (TryDequeue(out)) {
            return out;
        }
        return StdString();
    }

    Public Void StoreLog(ULong timestampMs, const StdString& message) override {}
};

#endif // ARDUINO_REMOTE_STORAGE_H
#endif // ARDUINO
