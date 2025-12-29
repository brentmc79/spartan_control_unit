# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Spartan Control Unit is a two-device ESP32 system for controlling Halo Spartan cosplay electronics:
- **Interface Device (ESP32 Dev):** TFT display with menu UI, sends commands via ESP-NOW
- **Receiver Device (ESP32-S3 Super Mini):** Controls addressable LEDs and fans based on received commands

## Build Commands

Build and upload use PlatformIO. Four environments exist:

```bash
# Build/upload Interface Device (main operation)
pio run -e esp32dev
pio run -e esp32dev -t upload

# Build/upload Receiver Device (main operation)
pio run -e esp32-s3-supermini
pio run -e esp32-s3-supermini -t upload

# Setup modes (for initial MAC address discovery)
pio run -e esp32dev-interface-setup -t upload
pio run -e esp32-s3-supermini-receiver-setup -t upload
```

## Architecture

### Device Mode Selection (Compile-Time)

The `DEVICE_MODE` build flag in `platformio.ini` determines behavior:
- `INTERFACE` → Full menu UI, sends state to receiver
- `RECEIVER` → Hardware control, receives commands
- `INTERFACE_SETUP` / `RECEIVER_SETUP` → MAC address pairing

### Key Components

**State Management (`include/state.h`, `src/state.cpp`):**
- Global `AppState` struct holds all configuration (visor mode/color/brightness, fan state, HUD style)
- Persisted to NVS via `Preferences` library

**Communication (`include/communication.h`, `src/communication.cpp`):**
- ESP-NOW protocol for low-latency wireless
- `CommandPayload` struct sent from Interface to Receiver
- Peer MAC addresses stored in NVS (saved during setup, loaded on boot)

**Menu System (`include/menu_system.h`, `src/menu_system.cpp`):**
- Configuration-driven hierarchical menus
- `MenuController` manages navigation, rendering, state updates
- Menu items have `onUpdate` callbacks for state changes + ESP-NOW transmission

**Hardware Control (receiver side in `main.cpp`):**
- LEDs: NeoPixel protocol on GPIO25, supports Solid/Pulsing/Flashing/Strobe modes
- Fans: GPIO26/27 for MOSFET control
- All animations are non-blocking (millis()-based)

### Data Flow

```
Button Press → MenuController → AppState update → CommandPayload via ESP-NOW → Receiver → Hardware
                                    ↓
                              Save to NVS
```

## Hardware Pinout

**Interface (ESP32 Dev):**
- GPIO12/13: Navigation buttons (uses OneButton library)
- GPIO15,18,23,2,4,32: TFT display (ST7789, 170x320)

**Receiver (ESP32-S3 Super Mini):**
- GPIO25: Addressable LED data (2 LEDs, GRB)
- GPIO26/27: Fan MOSFETs
- GPIO48: Onboard status LED

## First-Time Setup

To pair devices:
1. Upload `esp32dev-interface-setup` to interface device
2. Upload `esp32-s3-supermini-receiver-setup` to receiver device
3. Power both on - receiver broadcasts its MAC, interface receives and saves both addresses automatically
4. Wait for "Addresses saved!" confirmation on interface screen
5. Upload normal firmware (`esp32dev` and `esp32-s3-supermini`) to both devices

MAC addresses are stored in NVS under the "spartan-peers" namespace. If no addresses are found on boot, a warning is displayed.

## Extending the Menu

Add new `MenuItem` entries in `menu_system.cpp`. Each item specifies:
- Type: `SUBMENU`, `TOGGLE`, `CYCLE`, `ACTION`, `BACK`
- Options array for CYCLE items
- `onUpdate` callback for state changes
