#include "communication.h"
#include <esp_now.h>
#include <WiFi.h>

// MAC Address of the receiver device
uint8_t receiverAddress[] = {0x24, 0x58, 0x7C, 0xDB, 0x31, 0x3C};

// Callback when data is sent
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("\r\nLast Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void initCommunication() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_register_send_cb(onDataSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }
}

void sendStateUpdate() {
    CommandPayload payload;
    
    // Map the global appState to the payload
    payload.visorOn = appState.visorOn;
    payload.visorMode = appState.visorMode;
    payload.visorColor = appState.visorColor;
    payload.visorBrightness = appState.visorBrightness;
    payload.thermalsOn = appState.thermalsOn;

    esp_err_t result = esp_now_send(receiverAddress, (uint8_t *) &payload, sizeof(payload));

    if (result != ESP_OK) {
        Serial.println("Error sending the data");
    }
}
