#pragma once

#include "state.h"

// This is the data structure that will be sent over ESP-NOW.
// It is a subset of the AppState, containing only what the receiver needs.
struct CommandPayload {
    // Visor Settings
    bool visorOn;
    VisorMode visorMode;
    VisorColor visorColor;
    uint8_t visorBrightness;

    // Thermals
    bool thermalsOn;
};

// Initializes the ESP-NOW communication interface
void initCommunication();

// Sends the current application state to the receiver device
void sendStateUpdate();