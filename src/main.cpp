#include <Arduino.h>
#include <TFT_eSPI.h>
#include "pins.h"
#include "theme.h"
#include "menu_system.h"
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <esp_now.h>
#include <OneButton.h>

// --- Globals ---
TFT_eSPI tft = TFT_eSPI();
Adafruit_NeoPixel pixels(NUM_LEDS, LED_DATA, NEO_GRB + NEO_KHZ800);
OneButton buttonOne(BUTTON_1, true, true);
OneButton buttonTwo(BUTTON_2, true, true);

// Defined in menu_system.cpp
extern MenuItem mainMenuItems[];
extern const int mainMenuItemCount;

MenuController* menuController = nullptr;

// --- Forward Declarations ---
void handleNext();
void handleSelect();


void setupInterface() {
    Serial.println("Setting up interface");
    
    tft.begin();
    tft.setRotation(3);
    
    menuController = new MenuController(mainMenuItems, mainMenuItemCount, tft);

    // Attach button handlers
    buttonOne.attachClick(handleNext);
    buttonTwo.attachClick(handleSelect);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting setup");

    // For now, we are only focusing on the interface device
    setupInterface();
}

void loop() {
    buttonOne.tick();
    buttonTwo.tick();

    if (menuController) {
        menuController->render();
    }
    
    // A small delay to prevent flickering and excessive redraws
    delay(50); 
}

// --- Button Handlers ---
void handleNext() {
    if (menuController) {
        menuController->nextItem();
    }
}

void handleSelect() {
    if (menuController) {
        menuController->selectItem();
    }
}