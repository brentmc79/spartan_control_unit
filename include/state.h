#pragma once
#include <stdint.h>

// Enum for Visor Mode
enum class VisorMode : uint8_t { SOLID, FLASHING, PULSING };

// Enum for Visor Color
enum class VisorColor : uint8_t { WHITE, BLUE, GREEN, YELLOW, ORANGE, RED };

// Enum for HUD Style
enum class HudStyle : uint8_t { BIOMETRIC, RADAR, MATRIX };

// Enum for Boot Sequence
enum class BootSequence : uint8_t { UNSC_LOGO, PROGRESS_BAR };

struct AppState {
    // Visor Settings
    bool visorOn = false;
    VisorMode visorMode = VisorMode::SOLID;
    VisorColor visorColor = VisorColor::BLUE;
    uint8_t visorBrightness = 3; // Range 1-4

    // Thermals
    bool thermalsOn = false;

    // HUD
    HudStyle hudStyle = HudStyle::BIOMETRIC;

    // Settings
    BootSequence bootSequence = BootSequence::UNSC_LOGO;
};

// Declare a global instance of the state that can be accessed from any file
extern AppState appState;