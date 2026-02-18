#ifdef ARDUINO
#ifndef ARDUINO_REMOTE_STORAGE_H
#define ARDUINO_REMOTE_STORAGE_H

#include "IArduinoRemoteStorage.h"
#include "IFirebaseOperations.h"
#include "FirebaseOperations.h"
#include "ILogBuffer.h"
#include <ILogger.h>
#include <INetworkStatusProvider.h>
#include <Arduino.h>

#include <queue>
#include <mutex>

/* @Component */
class ArduinoRemoteStorage : public IArduinoRemoteStorage {

    Private IFirebaseOperationsPtr firebaseOperations;
    Private std::mutex firebaseOperationsMutex_;

    /* @Autowired */
    Private ILogBufferPtr logBuffer;

    /* @Autowired */
    Private ILoggerPtr logger;

    /* @Autowired */
    Private INetworkStatusProviderPtr networkStatusProvider_;

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

    /** Thread-safe: replaces firebaseOperations with a new FirebaseOperations instance. Proceeds only if internet is available. */
    Public Void ResetFirebaseOperations() {
        if (networkStatusProvider_ && !networkStatusProvider_->IsInternetConnected()) {
            logger->Verbose(Tag::Untagged, StdString("[ArduinoRemoteStorage] Skipping Firebase reset: no internet."));
            return;
        }
        logger->Info(Tag::Untagged, StdString("[ArduinoRemoteStorage] Resetting Firebase operations."));
        std::lock_guard<std::mutex> lock(firebaseOperationsMutex_);
        firebaseOperations = std::make_shared<FirebaseOperations>();
    }

    Public FirebaseOperationResult GetCommand(StdString& out) override {
        out.clear();
        if (TryDequeue(out)) {
            return FirebaseOperationResult::OperationSucceeded;
        }
        IFirebaseOperationsPtr ops;
        {
            std::lock_guard<std::mutex> lock(firebaseOperationsMutex_);
            ops = firebaseOperations;
        }
        if (!ops) {
            ResetFirebaseOperations();
            std::lock_guard<std::mutex> lock(firebaseOperationsMutex_);
            ops = firebaseOperations;
        }
        if (!ops) return FirebaseOperationResult::NotReady;
        if (ops->IsDirty()) {
            if (ops->IsOperationInProgress()) return FirebaseOperationResult::AnotherOperationInProgress;
            ResetFirebaseOperations();
            return FirebaseOperationResult::NotReady;
        }
        StdVector<StdString> commands;
        FirebaseOperationResult res = ops->RetrieveCommands(commands);
        if (res != FirebaseOperationResult::OperationSucceeded) return res;
        EnqueueAll(commands);
        if (TryDequeue(out)) {
            return FirebaseOperationResult::OperationSucceeded;
        }
        return FirebaseOperationResult::OperationSucceeded;
    }

    Public FirebaseOperationResult PublishLogs() override {
        StdMap<ULong, StdString> logs = logBuffer->TakeLogs();
        IFirebaseOperationsPtr ops;
        {
            std::lock_guard<std::mutex> lock(firebaseOperationsMutex_);
            ops = firebaseOperations;
        }
        if (!ops) {
            ResetFirebaseOperations();
            {
                std::lock_guard<std::mutex> lock(firebaseOperationsMutex_);
                ops = firebaseOperations;
            }
        }
        if (!ops) {
            logBuffer->AddLogs(logs);
            return FirebaseOperationResult::NotReady;
        }
        if (ops->IsDirty()) {
            if (ops->IsOperationInProgress()) {
                logBuffer->AddLogs(logs);
                return FirebaseOperationResult::AnotherOperationInProgress;
            }
            ResetFirebaseOperations();
            logBuffer->AddLogs(logs);
            return FirebaseOperationResult::NotReady;
        }
        FirebaseOperationResult res = ops->PublishLogs(logs);
        if (res != FirebaseOperationResult::OperationSucceeded) logBuffer->AddLogs(logs);
        return res;
    }
};

#endif // ARDUINO_REMOTE_STORAGE_H
#endif // ARDUINO
