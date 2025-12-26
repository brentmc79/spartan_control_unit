# Spartan Control Unit

This project is a control interface for accessory electronics embedded within a Spartan cosplay costume inspired by the Halo video game franchise. It is built using the [PlatformIO](https://platformio.org/) framework for the ESP32 microcontroller.

## Features

- UI navigation via 2 momentary buttons
- Toggle cooling fans on and off
- Control color, brightness, and display style of addressable LEDs
- Screensaver animation when idle
- Multiple boot-up animation sequences
- Configuration is saved and loaded on boot

## Hardware

- Ideaspark ESP32 1.9" TFT LCD Display
- 2 momentary buttons
- 2 Addressable LEDs in series
- 2 3010 DC 5v fans

### Purchase Links (Amazon)

- **Display:** [ESP32 1.9 inch TFT Display](https://a.co/d/jlKWzdy)
- **Buttons:** [Assorted Momentary Push Buttons](https://www.adafruit.com/product/3101)
- **LEDs:** [Inlightss WLED Controller Complete Kit](https://a.co/d/a8Dwrdc)
- **Fans:** [3010 DC 5v Fans](https://www.amazon.com/s?k=3010+dc+5v+fan)

## Software

### Libraries

- **TFT_eSPI:** [Documentation](https://github.com/Bodmer/TFT_eSPI)
- **Adafruit GFX Library:** [Documentation](https://learn.adafruit.com/adafruit-gfx-graphics-library)
- **Adafruit ST7735 and ST7789 Library:** [Documentation](https://github.com/adafruit/Adafruit-ST7735-Library)

### Framework

- **PlatformIO:** [Homepage](https://platformio.org/)

## Development

### Hardware Verification

To assist with wiring and debugging, a hardware verification function is included in the project. This function (`verifyHardwareConnections()`) guides you through checking the connections of the buttons and will be expanded to include other hardware components as they are integrated.

You can enable or disable this feature by modifying the `VERIFY_HARDWARE` boolean flag at the top of `src/main.cpp`:

```cpp
const bool VERIFY_HARDWARE = true; // Set to false to skip hardware verification
```

### Pin Definitions

All hardware pin assignments for the project are centralized in a dedicated header file: `include/pins.h`. This makes it easy to review and modify pin configurations in one place.

## Building and Running

This project is built and managed using PlatformIO.

-   **Build:** To build the project, use the command `pio run`.
-   **Upload:** To upload the compiled firmware to the ESP32 board, use the command `pio run --target upload`.
-   **Monitor:** To view the serial output from the board, use the command `pio device monitor`.
