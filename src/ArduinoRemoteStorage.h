#ifndef ARDUINO_REMOTE_STORAGE_H
#define ARDUINO_REMOTE_STORAGE_H

#include "IArduinoRemoteStorage.h"
#include <INetworkStatusProvider.h>
#include <Arduino.h>

/* @Component */
class ArduinoRemoteStorage : public IArduinoRemoteStorage {
    /* @Autowired */
    Private INetworkStatusProviderPtr networkStatusProvider_;
    Private Int storedWifiConnectionId_{0};

    /**
     * Ensures WiFi and internet are up and Firebase is bound to current WiFi id.
     * If WiFi not connected or internet not connected: DismissConnection(), return false.
     * If WiFi id changed: RefreshConnection(), update stored id, return true.
     * Otherwise return true.
     */
    Private Bool EnsureNetworkAndFirebaseMatch() {
        if (!networkStatusProvider_->IsWiFiConnected()) {
            //firebaseRequestManager->DismissConnection();
            return false;
        }
        if (!networkStatusProvider_->IsInternetConnected()) {
            //firebaseRequestManager->DismissConnection();
            return false;
        }
        Int currentId = networkStatusProvider_->GetWifiConnectionId();
        if (currentId != storedWifiConnectionId_) {
            Serial.print("[ArduinoRemoteStorage] WiFi connection id changed (");
            Serial.print(storedWifiConnectionId_);
            Serial.print(" -> ");
            Serial.print(currentId);
            Serial.println("); refreshing Firebase connection");
            //firebaseRequestManager->RefreshConnection();
            storedWifiConnectionId_ = currentId;
        }
        return true;
    }

    Public StdString GetCommand() override {
        if (!EnsureNetworkAndFirebaseMatch()) {
            return StdString();
        }
        return StdString();
    }

    Public Void StoreLog(const StdString&) override {}
};

#endif /* ARDUINO_REMOTE_STORAGE_H */
