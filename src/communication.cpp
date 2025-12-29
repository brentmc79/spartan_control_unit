#include "communication.h"
#include <Arduino.h>
#include <esp_now.h>

// Receiver MAC address - defined in main.cpp, loaded from Preferences
extern uint8_t recvAddress[];

void sendStateUpdate() {
    CommandPayload payload;

    // Map the global appState to the payload
    payload.visorOn = appState.visorOn;
    payload.visorMode = appState.visorMode;
    payload.visorColor = appState.visorColor;
    payload.visorBrightness = appState.visorBrightness;
    payload.thermalsOn = appState.thermalsOn;

    esp_err_t result = esp_now_send(recvAddress, (uint8_t *) &payload, sizeof(payload));

    if (result != ESP_OK) {
        Serial.println("Error sending the data");
    }
}
