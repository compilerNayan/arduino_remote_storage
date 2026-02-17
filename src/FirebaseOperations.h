#ifdef ARDUINO
#ifndef FIREBASEOPERATIONS_H
#define FIREBASEOPERATIONS_H

#include "IFirebaseOperations.h"
#include <INetworkStatusProvider.h>

#define Vector __FirebaseVector
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#undef Vector

#include <atomic>

/* @Component */
class FirebaseOperations : public IFirebaseOperations {
    /* @Autowired */
    Private INetworkStatusProviderPtr networkStatusProvider_;
    Private Int storedWifiConnectionId_{0};

    Private FirebaseData fbdo;
    Private FirebaseData fbdoDel;
    Private FirebaseAuth auth;
    Private FirebaseConfig config;
    Private Bool firebaseBegun = false;
    Private Bool streamBegun_ = false;
    Private std::atomic<bool> retrieving_{false};
    Private std::atomic<bool> refreshing_{false};
    Private std::atomic<bool> pendingRefresh_{false};

    Private Static const char* kDatabaseUrl() { return "https://smart-switch-da084-default-rtdb.asia-southeast1.firebasedatabase.app"; }
    Private Static const char* kLegacyToken() { return "Aj54Sf7eKxCaMIgTgEX4YotS8wbVpzmspnvK6X2C"; }
    Private Static const char* kPath() { return "/"; }
    Private Static const unsigned long kDeleteIntervalMs = 60000;
    Private unsigned long lastDeleteMillis_ = 0;

    Private Void EnsureFirebaseBegin() {
        if (firebaseBegun) return;
        config.database_url = kDatabaseUrl();
        config.signer.tokens.legacy_token = kLegacyToken();
        fbdo.setBSSLBufferSize(4096, 1024);
        fbdo.setResponseSize(2048);
        fbdoDel.setBSSLBufferSize(4096, 1024);
        fbdoDel.setResponseSize(2048);
        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);
        firebaseBegun = true;
        delay(500);
    }

    Private Bool EnsureStreamBegin() {
        if (streamBegun_) return true;
        if (!Firebase.RTDB.beginStream(&fbdo, kPath())) {
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
        Serial.print("[FirebaseOperations] RetrieveCommands failed: ");
        Serial.println(msg);
        pendingRefresh_.store(true);
    }

    Private Bool EnsureNetworkAndFirebaseMatch() {
        if (!networkStatusProvider_) {
            return false;
        }
        if (!networkStatusProvider_->IsWiFiConnected()) {
            DismissConnection();
            return false;
        }
        if (!networkStatusProvider_->IsInternetConnected()) {
            DismissConnection();
            return false;
        }
        Int currentId = networkStatusProvider_->GetWifiConnectionId();
        if (currentId != storedWifiConnectionId_) {
            Serial.print("[FirebaseOperations] WiFi connection id changed (");
            Serial.print(storedWifiConnectionId_);
            Serial.print(" -> ");
            Serial.print(currentId);
            Serial.println("); refreshing Firebase connection");
            RefreshConnection();
            storedWifiConnectionId_ = currentId;
        }
        return true;
    }

    Private Void RefreshConnection() {
        while (retrieving_.load(std::memory_order_relaxed)) {
            delay(10);
        }
        refreshing_.store(true, std::memory_order_release);
        struct ClearRefreshing {
            std::atomic<bool>& f;
            ~ClearRefreshing() { f.store(false); }
        } guard{refreshing_};

        Firebase.RTDB.endStream(&fbdo);
        streamBegun_ = false;
        firebaseBegun = false;
        delay(200);

        EnsureFirebaseBegin();
        if (Firebase.ready()) {
            EnsureStreamBegin();
        }
    }

    Private Void DismissConnection() {
        while (retrieving_.load(std::memory_order_relaxed)) {
            delay(10);
        }
        Firebase.RTDB.endStream(&fbdo);
        streamBegun_ = false;
        firebaseBegun = false;
    }

    /** Returns all key:value pairs from one stream read. No queue; returns full list. */
    Private StdVector<StdString> RetrieveCommandsFromFirebase() {
        StdVector<StdString> emptyResult;

        if (pendingRefresh_.exchange(false)) {
            RefreshConnection();
        }

        while (refreshing_.load(std::memory_order_relaxed)) {
            delay(10);
        }

        if (retrieving_.exchange(true)) {
            return emptyResult;
        }
        struct ClearFlag {
            std::atomic<bool>& f;
            ~ClearFlag() { f.store(false); }
        } guard{retrieving_};

        EnsureFirebaseBegin();
        if (!Firebase.ready()) {
            OnErrorAndScheduleRefresh("Firebase not ready");
            return emptyResult;
        }

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
                if (!Firebase.RTDB.deleteNode(&fbdoDel, kPath())) {
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
            if (!Firebase.RTDB.deleteNode(&fbdoDel, kPath())) {
                OnErrorAndScheduleRefresh(fbdoDel.errorReason().c_str());
            } else {
                lastDeleteMillis_ = now;
            }
        }

        StdVector<StdString> out(result.begin(), result.end());
        return out;
    }

    Public FirebaseOperations() = default;

    Public Virtual ~FirebaseOperations() override = default;

    Public StdVector<StdString> RetrieveCommands() override {
        if (!EnsureNetworkAndFirebaseMatch()) {
            return StdVector<StdString>();
        }
        return RetrieveCommandsFromFirebase();
    }
};

#endif // FIREBASEOPERATIONS_H
#endif // ARDUINO
