#ifdef ARDUINO
#ifndef FIREBASEOPERATIONS_H
#define FIREBASEOPERATIONS_H

#include "IFirebaseOperations.h"
#include <INetworkStatusProvider.h>
#include <ILogger.h>

#define Vector __FirebaseVector
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#undef Vector

#include <atomic>
#include <ctime>

class FirebaseOperations : public IFirebaseOperations {
    /* @Autowired */
    Private INetworkStatusProviderPtr networkStatusProvider_;
    /* @Autowired */
    Private ILoggerPtr logger;
    Private Int storedWifiConnectionId_{0};

    Private FirebaseData fbdo;
    Private FirebaseData fbdoDel;
    Private FirebaseData fbdoLog;
    Private FirebaseAuth auth;
    Private FirebaseConfig config;
    Private Bool firebaseBegun = false;
    Private Bool streamBegun_ = false;
    /** Only one of RetrieveCommands or PublishLogs may run at a time. */
    Private std::atomic<bool> operationInProgress_{false};
    /** When true, all public methods return default/empty without doing work. Set on any error; clear via ResetConnection(). */
    Private std::atomic<bool> dirty_{false};

    Private Static const char* kDatabaseUrl() { return "https://smart-switch-da084-default-rtdb.asia-southeast1.firebasedatabase.app"; }
    Private Static const char* kLegacyToken() { return "Aj54Sf7eKxCaMIgTgEX4YotS8wbVpzmspnvK6X2C"; }
    Private Static const char* kCommandsPath() { return "/commands"; }
    Private Static const unsigned long kDeleteIntervalMs = 60000;
    Private unsigned long lastDeleteMillis_ = 0;
    Private Static const char* kLogsPath() { return "/logs"; }
    /** UTC ms when millis() was 0; set on first PublishLogs when time() is available. */
    Private ULong epochOffsetMs_{0};

    Private Void EnsureFirebaseBegin() {
        if (firebaseBegun) return;
        config.database_url = kDatabaseUrl();
        config.signer.tokens.legacy_token = kLegacyToken();
        fbdo.setBSSLBufferSize(4096, 1024);
        fbdo.setResponseSize(2048);
        fbdoDel.setBSSLBufferSize(4096, 1024);
        fbdoDel.setResponseSize(2048);
        fbdoLog.setBSSLBufferSize(4096, 1024);
        fbdoLog.setResponseSize(2048);
        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);
        firebaseBegun = true;
        delay(500);
    }

    Private Bool EnsureStreamBegin() {
        if (streamBegun_) return true;
        if (!Firebase.RTDB.beginStream(&fbdo, kCommandsPath())) {
            return false;
        }
        streamBegun_ = true;
        return true;
    }

    Private Static Void ParseJsonToKeyValuePairs(const StdString& jsonStr, StdList<StdString>& out, StdList<StdString>& outKeys) {
        DynamicJsonDocument doc(2048);
        DeserializationError err = deserializeJson(doc, jsonStr);
        if (err) return;
        if (!doc.is<JsonObject>()) return;
        JsonObject root = doc.as<JsonObject>();
        for (JsonPair p : root) {
            StdString key(p.key().c_str());
            outKeys.push_back(key);
            StdString value;
            if (p.value().is<const char*>())
                value = p.value().as<const char*>();
            else if (p.value().is<int>())
                value = std::to_string(p.value().as<int>());
            else if (p.value().is<bool>())
                value = p.value().as<bool>() ? "true" : "false";
            else if (p.value().is<double>())
                value = std::to_string(p.value().as<double>());
            else {
                String s;
                serializeJson(p.value(), s);
                value = StdString(s.c_str());
            }
            out.push_back(key + ":" + value);
        }
    }

    Private Void OnErrorAndScheduleRefresh(const char* msg) {
        logger->Error(Tag::Untagged, StdString(std::string("[FirebaseOperations] RetrieveCommands failed: ") + msg));
        dirty_.store(true);
    }

    Private Bool EnsureNetworkAndFirebaseMatch() {
        if (!networkStatusProvider_) {
            dirty_.store(true);
            return false;
        }
        if (!networkStatusProvider_->IsWiFiConnected()) {
            dirty_.store(true);
            return false;
        }
        if (!networkStatusProvider_->IsInternetConnected()) {
            dirty_.store(true);
            return false;
        }
        Int currentId = networkStatusProvider_->GetWifiConnectionId();
        if (currentId != storedWifiConnectionId_) {
            if (storedWifiConnectionId_ != 0) {
                logger->Info(Tag::Untagged, StdString("[FirebaseOperations] WiFi connection id changed (" + std::to_string(storedWifiConnectionId_) + " -> " + std::to_string(currentId) + "); refreshing Firebase connection"));
                dirty_.store(true);
            }
            storedWifiConnectionId_ = currentId;
        }
        return true;
    }

    /** Returns false if dirty, network/Firebase mismatch, or Firebase not ready. Otherwise ensures Firebase begun and returns true. */
    Private Bool EnsureReady() {
        if (dirty_.load(std::memory_order_relaxed)) return false;
        if (!EnsureNetworkAndFirebaseMatch()) return false;
        EnsureFirebaseBegin();
        return Firebase.ready();
    }

    /** Convert timestampMs (millis since boot) to Firebase-safe key "2026-02-17T13-05-00_123Z" (underscore, no period; period is invalid in Firebase paths). */
    Private StdString MillisToIso8601(ULong timestampMs) {
        if (epochOffsetMs_ == 0) {
            time_t now = time(nullptr);
            if (now > 0) {
                epochOffsetMs_ = (ULong)now * 1000 - timestampMs;
            } else {
                return "millis_" + std::to_string(timestampMs);
            }
        }
        ULong utcMs = epochOffsetMs_ + timestampMs;
        time_t sec = static_cast<time_t>(utcMs / 1000);
        unsigned int ms = static_cast<unsigned int>(utcMs % 1000);
        struct tm* t = gmtime(&sec);
        if (!t) return "millis_" + std::to_string(timestampMs);
        char buf[32];
        size_t n = strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", t);
        if (n == 0) return "millis_" + std::to_string(timestampMs);
        char out[40];
        snprintf(out, sizeof(out), "%s_%03uZ", buf, ms);
        return StdString(out);
    }

    /** Returns all key:value pairs from one stream read. No queue; returns full list. Call only while operationInProgress_ is held. */
    Private StdVector<StdString> RetrieveCommandsFromFirebase() {
        StdVector<StdString> emptyResult;

        if (!EnsureStreamBegin()) {
            OnErrorAndScheduleRefresh("beginStream failed");
            return emptyResult;
        }

        if (!Firebase.RTDB.readStream(&fbdo)) {
            OnErrorAndScheduleRefresh(fbdo.errorReason().c_str());
            return emptyResult;
        }

        if (!fbdo.streamAvailable()) {
            unsigned long now = millis();
            if (now - lastDeleteMillis_ >= kDeleteIntervalMs) {
                if (!Firebase.RTDB.deleteNode(&fbdoDel, kCommandsPath())) {
                    OnErrorAndScheduleRefresh(fbdoDel.errorReason().c_str());
                } else {
                    lastDeleteMillis_ = now;
                }
            }
            return emptyResult;
        }

        String raw = fbdo.to<String>();
        StdString payload(raw.c_str());

        StdList<StdString> result;
        StdList<StdString> keysReceived;
        ParseJsonToKeyValuePairs(payload, result, keysReceived);

        unsigned long now = millis();
        if (now - lastDeleteMillis_ >= kDeleteIntervalMs) {
            if (!Firebase.RTDB.deleteNode(&fbdoDel, kCommandsPath())) {
                OnErrorAndScheduleRefresh(fbdoDel.errorReason().c_str());
            } else {
                lastDeleteMillis_ = now;
            }
        }

        StdVector<StdString> out(result.begin(), result.end());
        return out;
    }

    Public FirebaseOperations() = default;

    Public Virtual ~FirebaseOperations() override {
        dirty_.store(true);
        const ULong kShutdownWaitMs = 3000u;
        ULong deadline = (ULong)millis() + kShutdownWaitMs;
        while (operationInProgress_.load(std::memory_order_relaxed) && (ULong)millis() < deadline) {
            delay(10);
        }
        if (streamBegun_ || firebaseBegun) {
            Firebase.RTDB.endStream(&fbdo);
            streamBegun_ = false;
            firebaseBegun = false;
            delay(100);
        }
    }

    Public FirebaseOperationResult RetrieveCommands(StdVector<StdString>& out) override {
        out.clear();
        if (operationInProgress_.exchange(true)) {
            return FirebaseOperationResult::AnotherOperationInProgress;
        }
        struct ClearOp {
            std::atomic<bool>& f;
            ~ClearOp() { f.store(false); }
        } guard{operationInProgress_};
        if (!EnsureReady()) return FirebaseOperationResult::NotReady;
        out = RetrieveCommandsFromFirebase();
        return FirebaseOperationResult::OperationSucceeded;
    }

    Public FirebaseOperationResult PublishLogs(const StdMap<ULong, StdString>& logs) override {
        if (operationInProgress_.exchange(true)) {
            return FirebaseOperationResult::AnotherOperationInProgress;
        }
        struct ClearOp {
            std::atomic<bool>& f;
            ~ClearOp() { f.store(false); }
        } guard{operationInProgress_};
        if (!EnsureReady()) return FirebaseOperationResult::NotReady;
        Bool ok = true;
        for (const auto& pair : logs) {
            StdString key = MillisToIso8601(pair.first);
            const StdString& message = pair.second;
            if (message.empty()) continue;
            StdString path = StdString(kLogsPath()) + "/" + key;
            if (!Firebase.RTDB.setString(&fbdoLog, path.c_str(), message.c_str())) {
                logger->Error(Tag::Untagged, StdString("[FirebaseOperations] PublishLogs failed for " + key + ": " + StdString(fbdoLog.errorReason().c_str())));
                ok = false;
            }
        }
        return ok ? FirebaseOperationResult::OperationSucceeded : FirebaseOperationResult::Failed;
    }

    /** Returns true if RetrieveCommands or PublishLogs is currently running. */
    Public Virtual Bool IsOperationInProgress() const override {
        return operationInProgress_.load(std::memory_order_relaxed);
    }

    /** Returns true if the instance is dirty (e.g. after an error); public methods will return default/empty until reset. */
    Public Virtual Bool IsDirty() const override {
        return dirty_.load(std::memory_order_relaxed);
    }
};

#endif // FIREBASEOPERATIONS_H
#endif // ARDUINO
