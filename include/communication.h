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

// This struct is used during the setup phase to exchange MAC addresses
struct SetupPayload {
    char macAddress[18]; // MAC addresses are 17 characters long + null terminator
};

// Initializes the ESP-NOW communication interface
void initCommunication();

// Sends the current application state to the receiver device
void sendStateUpdate();