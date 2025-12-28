#include <Arduino.h>
#include <TFT_eSPI.h>
#include "pins.h"
#include "theme.h"
#include "menu_system.h"
#include "communication.h"
#include <Adafruit_NeoPixel.h>
#include <OneButton.h>

// --- Globals ---
TFT_eSPI tft = TFT_eSPI();
Adafruit_NeoPixel pixels(NUM_LEDS, LED_DATA, NEO_GRB + NEO_KHZ800);
OneButton buttonOne(BUTTON_1, true, true);
OneButton buttonTwo(BUTTON_2, true, true);

MenuController* menuController = nullptr;

// --- Forward Declarations ---
void handleNext();
void handleSelect();

void setupInterface() {
    Serial.println("Setting up interface");
    
    tft.begin();
    tft.setRotation(3);
    
    // Initialize the menu system
    menuController = new MenuController(mainMenuItems, mainMenuItemCount, tft);

    // Initialize ESP-NOW communication
    initCommunication();

    // Attach button handlers
    buttonOne.attachClick(handleNext);
    buttonTwo.attachClick(handleSelect);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting SpartanOS Interface...");

    setupInterface();
    
    // Send the initial state to the receiver on boot
    sendStateUpdate();
}

void loop() {
    buttonOne.tick();
    buttonTwo.tick();

    if (menuController) {
        menuController->render();
    }
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