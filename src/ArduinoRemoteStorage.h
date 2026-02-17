#ifdef ARDUINO
#ifndef ARDUINO_REMOTE_STORAGE_H
#define ARDUINO_REMOTE_STORAGE_H

#include "IArduinoRemoteStorage.h"
#include "IFirebaseOperations.h"
#include "FirebaseOperations.h"
#include "ILogBuffer.h"
#include <Arduino.h>

#include <queue>
#include <mutex>

/* @Component */
class ArduinoRemoteStorage : public IArduinoRemoteStorage {

    Private IFirebaseOperationsPtr firebaseOperations;
    Private std::mutex firebaseOperationsMutex_;

    /* @Autowired */
    Private ILogBufferPtr logBuffer;

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

    Public ArduinoRemoteStorage() {
        ResetFirebaseOperations();
    }

    Public Virtual ~ArduinoRemoteStorage() override = default;

    /** Thread-safe: replaces firebaseOperations with a new FirebaseOperations instance. */
    Public Void ResetFirebaseOperations() {
        std::lock_guard<std::mutex> lock(firebaseOperationsMutex_);
        firebaseOperations = std::make_shared<FirebaseOperations>();
    }

    Public StdString GetCommand() override {
        StdString out;
        if (TryDequeue(out)) {
            return out;
        }
        IFirebaseOperationsPtr ops;
        {
            std::lock_guard<std::mutex> lock(firebaseOperationsMutex_);
            ops = firebaseOperations;
        }
        if (!ops) return StdString();
        if (ops->IsDirty()) {
            if (ops->IsOperationInProgress()) return StdString();
            ResetFirebaseOperations();
            return StdString();
        }
        StdVector<StdString> commands = ops->RetrieveCommands();
        EnqueueAll(commands);
        if (TryDequeue(out)) {
            return out;
        }
        return StdString();
    }

    Public Bool PublishLogs() override {
        StdMap<ULong, StdString> logs = logBuffer->TakeLogs();
        if (logs.empty()) return true;
        IFirebaseOperationsPtr ops;
        {
            std::lock_guard<std::mutex> lock(firebaseOperationsMutex_);
            ops = firebaseOperations;
        }
        if (!ops) {
            logBuffer->AddLogs(logs);
            return false;
        }
        if (ops->IsDirty()) {
            if (ops->IsOperationInProgress()) {
                logBuffer->AddLogs(logs);
                return false;
            }
            ResetFirebaseOperations();
            logBuffer->AddLogs(logs);
            return false;
        }
        Bool ok = ops->PublishLogs(logs);
        if (!ok) logBuffer->AddLogs(logs);
        return ok;
    }
};

#endif // ARDUINO_REMOTE_STORAGE_H
#endif // ARDUINO
