# Spartan Control Unit

## Project Overview

This is a C++ project for an ESP32 microcontroller using the PlatformIO framework. It's a control interface for accessory electronics embedded within a Spartan cosplay costume inspired by the Halo video game franchise.

## Features

- UI navigation via 2 momentary buttons where one button traverses UI elements, and the other button selects the active UI element
- Toggle cooling fans on and off
- Controlling the color, brightness, and display style of a pair of addressable LEDs
- Configuring a screensaver animation to play when idle
- Displaying one of several boot-up animation sequences
- Storing selected configuration to be loaded on next boot up

## System Architecture

This project is designed to run on two separate ESP32 devices that communicate with each other. The same codebase is used for both devices, but it is compiled with different configurations based on the target device.

### Interface Device

*   **MCU:** ESP32 Dev Module with integrated TFT LCD
*   **Peripherals:** Two momentary push buttons for UI navigation
*   **Role:** This device acts as the user interface. It has a screen and buttons to control the system.

### Receiver Device

*   **MCU:** ESP32-S3 Super Mini
*   **Peripherals:** Addressable LEDs and a 5V DC fan (controlled via a transistor)
*   **Role:** This device receives commands from the Interface Device to control the LEDs and fan.

## Hardware

- Ideaspark ESP32 1.9" TFT LCD Display
- 2 momentary buttons
- 2 Addressable LEDs in series
- 2 3010 DC 5v fans

### Pinout

![Pinout Diagram](docs/pinout.png)

### Wiring

Here are the suggested connections for the accessories.

#### Momentary Buttons

For each button, a 10k Ohm pull-down resistor is required.

*   **Button 1:**
    *   Connect one leg to **3.3V**.
    *   Connect the other leg to **GPIO12**.
    *   Connect a 10k Ohm resistor from **GPIO12** to **GND**.
*   **Button 2:**
    *   Connect one leg to **3.3V**.
    *   Connect the other leg to **GPIO13**.
    *   Connect a 10k Ohm resistor from **GPIO13** to **GND**.

#### LEDs (Addressable)

*   **Data Pin:** Connect to `GPIO25` on the ESP32.
    *   **Note:** Since the addressable LEDs are 5V and the ESP32's GPIOs are 3.3V, a logic level shifter is recommended between `GPIO25` and the LED data pin to ensure reliable data transmission.
*   **Power:** Connect the LED strip's 5V pin to an external 5V power supply.
*   **Ground:** Connect the LED strip's GND pin to the same ground as the ESP32 and the 5V power supply.

#### Fans (5V)

To control the 5V fans, you'll need a transistor (like a MOSFET) for each fan, as the ESP32's GPIO pins cannot provide enough power.

*   **Fan 1 Control:** `GPIO26`
*   **Fan 2 Control:** `GPIO27`

##### Wiring with N-Channel MOSFETs (for each fan):

1.  Connect the **Gate** of the MOSFET to the GPIO pin (e.g., `GPIO26`).
2.  Connect the **Source** of the MOSFET to Ground (GND).
3.  Connect the **Drain** of the MOSFET to the negative (black) wire of the fan.
4.  Connect the positive (red) wire of the fan to the 5V power supply.

## Project Structure

-   `src/main.cpp`: The main entry point of the application. It initializes the display and calls the animation functions.
-   `src/unsc_logo.cpp` and `include/unsc_logo.h`: These files contain the bitmap data for the UNSC logo and the function to display and scroll it.
-   `src/loading_animations.cpp` and `include/loading_animations.h`: These files contain several different loading animations that can be used in the boot sequence.
-   `platformio.ini`: The PlatformIO configuration file, which specifies the board, framework, and library dependencies.

## Building and Running

This project is built and managed using PlatformIO.

-   **Build:** To build the project, use the command `pio run`.
-   **Upload:** To upload the compiled firmware to the ESP32 board, use the command `pio run --target upload`.
-   **Monitor:** To view the serial output from the board, use the command `pio device monitor`.

### Compiling for a specific device

The `platformio.ini` file has two environments defined:

*   `esp32dev`: Builds the firmware for the **Interface Device**. It sets the `DEVICE_MODE` build flag to `DeviceMode::INTERFACE`.
*   `esp32-s3-supermini`: Builds the firmware for the **Receiver Device**. It sets the `DEVICE_MODE` build flag to `DeviceMode::RECEIVER`.

To build for a specific device, you can use the `-e` flag with the PlatformIO CLI. For example:

*   Build for Interface Device: `pio run -e esp32dev`
*   Build for Receiver Device: `pio run -e esp32-s3-supermini`

## Development

### Hardware Verification

To assist with wiring and debugging, a hardware verification function is included in the project. This function (`verifyHardwareConnections()`) guides you through checking the connections of the buttons and will be expanded to include other hardware components as they are integrated.

You can enable or disable this feature by modifying the `VERIFY_HARDWARE` boolean flag at the top of `src/main.cpp`:

```cpp
const bool VERIFY_HARDWARE = true; // Set to false to skip hardware verification
```

### Development Conventions

The code is written in C++ and follows the Arduino framework conventions. The code is organized into separate files for different functionalities, which is a good practice for embedded projects. The use of header files helps to keep the code modular and easy to maintain.

There are no specific linting or formatting rules defined in the project, but the code is well-formatted and easy to read.

Any ui/navigation code should be configuration-driven, allowing additional views, menu items, etc to added easily.