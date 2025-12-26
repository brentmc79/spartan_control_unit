#ifndef PINS_H
#define PINS_H

#define LCD_MOSI 23 // ESP32 D23
#define LCD_SCLK 18 // ESP32 D18
#define LCD_CS   15 // ESP32 D15
#define LCD_DC    2 // ESP32 D2
#define LCD_RST   4 // ESP32 D4
#define LCD_BLK  32 // ESP32 D32

#define BUTTON_1 12
#define BUTTON_2 13

#define LED_DATA 25 // GPIO for addressable LEDs
#define FAN_1_CTRL 26 // GPIO for Fan 1 control
#define FAN_2_CTRL 27 // GPIO for Fan 2 control

#endif // PINS_H
