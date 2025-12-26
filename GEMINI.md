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

## Hardware

- Ideaspark ESP32 1.9" TFT LCD Display
- 2 momentary buttons
- 2 Addressable LEDs in series
- 2 3010 DC 5v fans

### Pinout

![Pinout Diagram](docs/pinout.png)

### Wiring

Here are the suggested connections for the accessories.

### Momentary Buttons

*   **Button 1:** Connect one leg to **GPIO36** and the other leg to **GND**.
*   **Button 2:** Connect one leg to **GPIO39** and the other leg to **GND**.

### LEDs (Addressable)

*   **Data Pin:** Connect to `GPIO25` on the ESP32.
    *   **Note:** Since the addressable LEDs are 5V and the ESP32's GPIOs are 3.3V, a logic level shifter is recommended between `GPIO25` and the LED data pin to ensure reliable data transmission.
*   **Power:** Connect the LED strip's 5V pin to an external 5V power supply.
*   **Ground:** Connect the LED strip's GND pin to the same ground as the ESP32 and the 5V power supply.

### Fans (5V)

To control the 5V fans, you'll need a transistor (like a MOSFET) for each fan, as the ESP32's GPIO pins cannot provide enough power.

*   **Fan 1 Control:** `GPIO26`
*   **Fan 2 Control:** `GPIO27`

#### Wiring with N-Channel MOSFETs (for each fan):

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

## Development Conventions

The code is written in C++ and follows the Arduino framework conventions. The code is organized into separate files for different functionalities, which is a good practice for embedded projects. The use of header files helps to keep the code modular and easy to maintain.

There are no specific linting or formatting rules defined in the project, but the code is well-formatted and easy to read.

Any ui/navigation code should be configuration-driven, allowing additional views, menu items, etc to added easily.
