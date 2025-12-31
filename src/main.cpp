#include <Arduino.h>
#include <TFT_eSPI.h>
#include "pins.h"
#include "theme.h"
#include "layout.h"
#include "menu_system.h"
#include "communication.h"
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <esp_now.h>
#include <OneButton.h>
#include <Preferences.h>
#include <memory>

enum class DeviceMode : uint8_t {
  INTERFACE,
  INTERFACE_SETUP,
  RECEIVER,
  RECEIVER_SETUP,
};

#ifndef DEVICE_MODE
// Provide a default value if the flag is not defined
#define DEVICE_MODE DeviceMode::INTERFACE
#endif

const bool isInterface = DEVICE_MODE == DeviceMode::INTERFACE;
const bool isInterfaceSetup = DEVICE_MODE == DeviceMode::INTERFACE_SETUP;
const bool isReceiver = DEVICE_MODE == DeviceMode::RECEIVER;
const bool isReceiverSetup = DEVICE_MODE == DeviceMode::RECEIVER_SETUP;

// --- Globals ---
TFT_eSPI tft = TFT_eSPI();
Adafruit_NeoPixel pixels(NUM_LEDS, LED_DATA, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels2(NUM_LEDS, LED_DATA_2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel onboardLED(1, 48, NEO_GRB + NEO_KHZ800);
OneButton buttonOne(BUTTON_1, true, true);
OneButton buttonTwo(BUTTON_2, true, true);
OneButton buttonThree(BUTTON_3, true, true);

Preferences preferences;

std::unique_ptr<MenuController> menuController;

// Defined in menu_system.cpp
extern MenuItem mainMenuItems[];
extern const int mainMenuItemCount;

// Peer MAC addresses - loaded from Preferences after running setup firmware
// These defaults are placeholders; run setup firmware to pair devices
uint8_t sendAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Interface MAC (set by setup)
uint8_t recvAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Receiver MAC (set by setup)
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

String macAddress; // This will hold the MAC address string

// Receiver watchdog - tracks last message time to detect connection loss
unsigned long lastMessageTime = 0;

// Interface heartbeat - tracks last time state was sent to receiver
unsigned long lastHeartbeatTime = 0;

// Screen saver - tracks last interaction time and active state
unsigned long lastInteractionTime = 0;
bool screenSaverActive = false;

// --- Forward Declarations ---
void handleNext();
void handlePrevious();
void handleSelect();
void setupInterface();
void setupReceiver();
void setupInterfaceSetup();
void setupReceiverSetup();
void setupEspComms();
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
void updateHardwareState(const CommandPayload& payload);
void saveAppState();
void loadAppState();
void parseMacAddress(const char* macStr, uint8_t* macBytes);
void savePeerAddresses();
bool loadPeerAddresses();
void updateMenuFromState(); // Defined in menu_system.cpp
void pulseLeds();
void flashLeds();
void strobeLeds();
uint32_t getVisorColorValue(VisorColor color);
void resetToSafeState();
void renderScreenSaver();
void resetIdleTimer();
void exitScreenSaver();


void setupInterface() {
    Serial.println("Setting up interface");
    
    tft.begin();
    tft.setRotation(3);

    // Initialize the menu system
    menuController = std::make_unique<MenuController>(mainMenuItems, mainMenuItemCount, tft);

    // Attach button handlers
    buttonOne.attachClick(handleNext);
    buttonTwo.attachClick(handleSelect);
    buttonThree.attachClick(handlePrevious);

    // Initialize idle timer for screen saver
    resetIdleTimer();
}

void setupReceiver() {
  Serial.println("Setting up receiver");

  pixels.begin();
  pixels.clear();
  pixels.show();

  pixels2.begin();
  pixels2.clear();
  pixels2.show();

  onboardLED.begin();
  onboardLED.clear();
  onboardLED.show();

  pinMode(FAN_1_CTRL, OUTPUT);
  delay(100); // Allow pins to settle
}

void setupInterfaceSetup() {
  Serial.println("Setting up interface setup");

  tft.begin();
  tft.setRotation(1);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.print("Interface MAC:");
  tft.setCursor(0, 20);
  tft.print(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

void setupReceiverSetup() {
  Serial.println("Setting up receiver setup");

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void setupEspComms() {
  // Load saved peer addresses (falls back to zeros if not found)
  bool hasAddresses = loadPeerAddresses();

  if (!hasAddresses) {
    Serial.println("WARNING: No peer addresses configured!");
    Serial.println("Run the setup firmware to pair devices.");

    if (isInterface) {
      // Show warning on display
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_YELLOW);
      tft.setTextSize(2);
      tft.setCursor(10, 60);
      tft.print("No peer configured!");
      tft.setCursor(10, 90);
      tft.print("Run setup firmware");
      tft.setCursor(10, 110);
      tft.print("to pair devices.");
      tft.setTextColor(TFT_WHITE);
      delay(3000);
    }
  }

  // Init ESP-NOW
  Serial.println("Initializing ESP-NOW");
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callbacks
  Serial.println("Registering callbacks");
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // Register peer - Interface sends to Receiver, Receiver sends to Interface
  Serial.println("Registering peer");
  esp_now_peer_info_t peerInfo = {};
  if (isInterface) {
    memcpy(peerInfo.peer_addr, recvAddress, 6);
  } else {
    memcpy(peerInfo.peer_addr, sendAddress, 6);
  }
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  Serial.println("Adding peer");
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.print("Awaiting messages at ");
  Serial.println(WiFi.macAddress());
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting SpartanOS...");

    loadAppState();
    updateMenuFromState();

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    macAddress = WiFi.macAddress();

    if (isInterfaceSetup) {
      setupInterfaceSetup();
      return;
    }

    if (isReceiverSetup) {
      setupReceiverSetup();
    }

    if (isInterface) {
      setupInterface();
    }

    if (isReceiver) {
      setupReceiver();
    }
    
    if (isInterface || isReceiver) {
      setupEspComms();
    }
    
    // Send the initial state to the receiver on boot
    sendStateUpdate();
}

void loop() {
    if (isReceiverSetup) {
      SetupPayload setupPayload;
      strncpy(setupPayload.macAddress, WiFi.macAddress().c_str(), sizeof(setupPayload.macAddress) - 1);
      setupPayload.macAddress[sizeof(setupPayload.macAddress) - 1] = '\0';
      esp_now_send(broadcastAddress, (uint8_t *) &setupPayload, sizeof(setupPayload));
      Serial.println("Sent MAC address");
      delay(2000);
      return;
    }

    if (isInterfaceSetup) {
      // Keep alive to receive messages
      delay(100);
      return;
    }

    buttonOne.tick();
    buttonTwo.tick();
    buttonThree.tick();

    // Screen saver logic (interface only)
    if (isInterface) {
        if (!screenSaverActive && (millis() - lastInteractionTime >= SCREENSAVER_TIMEOUT_MS)) {
            screenSaverActive = true;
            renderScreenSaver();
        }

        if (screenSaverActive) {
            renderScreenSaver();
        } else if (menuController) {
            menuController->render();
        }
    }

    // Interface heartbeat - periodically send state to keep receiver's watchdog happy
    if (isInterface) {
        if (millis() - lastHeartbeatTime >= HEARTBEAT_INTERVAL_MS) {
            lastHeartbeatTime = millis();
            sendStateUpdate();
        }
    }

    // Receiver watchdog - reset to safe state if no messages received
    if (isReceiver && lastMessageTime > 0) {
        if (millis() - lastMessageTime > RECEIVER_TIMEOUT_MS) {
            if (appState.visorOn || appState.thermalsOn) {
                resetToSafeState();
            }
        }
    }

    if (isReceiver && appState.visorOn && appState.visorMode == VisorMode::PULSING) {
        pulseLeds();
    } else if (isReceiver && appState.visorOn && appState.visorMode == VisorMode::FLASHING) {
        flashLeds();
    } else if (isReceiver && appState.visorOn && appState.visorMode == VisorMode::STROBE) {
        strobeLeds();
    }
}

// --- Button Handlers ---
void handleNext() {
    resetIdleTimer();
    if (screenSaverActive) {
        exitScreenSaver();
        return;
    }
    if (menuController) {
        menuController->nextItem();
    }
}

void handlePrevious() {
    resetIdleTimer();
    if (screenSaverActive) {
        exitScreenSaver();
        return;
    }
    if (menuController) {
        menuController->prevItem();
    }
}

void handleSelect() {
    resetIdleTimer();
    if (screenSaverActive) {
        exitScreenSaver();
        return;
    }
    if (menuController) {
        menuController->selectItem();
    }
}

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (isReceiver) {
    if (len == sizeof(CommandPayload)) {
      CommandPayload payload;
      memcpy(&payload, incomingData, sizeof(CommandPayload));
      Serial.print("Bytes received (CommandPayload): ");
      Serial.println(len);
      // Process CommandPayload
      appState.visorOn = payload.visorOn;
      appState.visorMode = payload.visorMode;
      appState.visorColor = payload.visorColor;
      appState.visorBrightness = payload.visorBrightness;
      appState.thermalsOn = payload.thermalsOn;
      updateHardwareState(payload);

      // Reset watchdog timer
      lastMessageTime = millis();

      // Blink onboard LED on receiver for incoming message
      onboardLED.setPixelColor(0, onboardLED.Color(0, 0, 255)); // Blue color
      onboardLED.setBrightness(10);
      onboardLED.show();
      delay(50);
      onboardLED.clear();
      onboardLED.show();
    } else {
      Serial.print("Received unexpected payload size for receiver: ");
      Serial.println(len);
    }
  } else if (isInterfaceSetup) {
    if (len == sizeof(SetupPayload)) {
      SetupPayload setupPayload;
      memcpy(&setupPayload, incomingData, sizeof(SetupPayload));
      Serial.print("Bytes received (SetupPayload): ");
      Serial.println(len);
      Serial.print("Receiver MAC: ");
      Serial.println(setupPayload.macAddress);

      // Parse and save both MAC addresses
      parseMacAddress(WiFi.macAddress().c_str(), sendAddress);
      parseMacAddress(setupPayload.macAddress, recvAddress);
      savePeerAddresses();

      tft.setCursor(0, 60);
      tft.print("Receiver MAC:");
      tft.setCursor(0, 80);
      tft.print(setupPayload.macAddress);
      tft.setCursor(0, 120);
      tft.setTextColor(TFT_GREEN);
      tft.print("Addresses saved!");
      tft.setCursor(0, 140);
      tft.print("Upload normal firmware");
      tft.setTextColor(TFT_WHITE);
    } else {
      Serial.print("Received unexpected payload size for interface setup: ");
      Serial.println(len);
    }
  } else {
    // For other modes, just print raw data (or ignore)
    Serial.print("Bytes received (raw): ");
    Serial.println(len);
  }
}

// Converts a VisorColor enum to its RGB color value
uint32_t getVisorColorValue(VisorColor color) {
    switch (color) {
        case VisorColor::WHITE:  return pixels.Color(255, 255, 255);
        case VisorColor::BLUE:   return pixels.Color(0, 0, 255);
        case VisorColor::GREEN:  return pixels.Color(0, 255, 0);
        case VisorColor::YELLOW: return pixels.Color(255, 255, 0);
        case VisorColor::ORANGE: return pixels.Color(255, 128, 0);
        case VisorColor::RED:    return pixels.Color(255, 0, 0);
        default:                 return pixels.Color(0, 0, 0);
    }
}

// Function to update the hardware state based on the CommandPayload
void updateHardwareState(const CommandPayload& payload) {
  // Update LEDs
  if (payload.visorOn) {
    uint32_t color = getVisorColorValue(payload.visorColor);
    if (payload.visorMode != VisorMode::PULSING) {
      pixels.setBrightness(payload.visorBrightness * BRIGHTNESS_STEP);
      pixels2.setBrightness(payload.visorBrightness * BRIGHTNESS_STEP);
    }
    pixels.fill(color, 0, NUM_LEDS);
    pixels2.fill(color, 0, NUM_LEDS);
  } else {
    pixels.clear();
    pixels2.clear();
  }
  pixels.show();
  pixels2.show();

  // Update Fans
  digitalWrite(FAN_1_CTRL, payload.thermalsOn ? HIGH : LOW);
  // Assuming FAN_2_CTRL also exists and follows thermalsOn
  // digitalWrite(FAN_2_CTRL, payload.thermalsOn ? HIGH : LOW);
}

// Resets hardware to safe state when connection is lost
void resetToSafeState() {
  Serial.println("WARNING: Connection timeout - resetting to safe state");

  // Turn off fans immediately (safety first)
  digitalWrite(FAN_1_CTRL, LOW);

  // Update app state to reflect safe state
  appState.visorOn = false;
  appState.thermalsOn = false;

  // Set onboard LED red to indicate timeout
  onboardLED.setPixelColor(0, onboardLED.Color(255, 0, 0));
  onboardLED.setBrightness(10);
  onboardLED.show();

  // Phase 1: Flash bright red for 5 seconds
  uint32_t red = pixels.Color(255, 0, 0);
  unsigned long flashStart = millis();
  bool ledOn = true;

  pixels.setBrightness(MAX_BRIGHTNESS);
  pixels2.setBrightness(MAX_BRIGHTNESS);
  while (millis() - flashStart < SHUTDOWN_FLASH_DURATION_MS) {
    if (ledOn) {
      pixels.fill(red, 0, NUM_LEDS);
      pixels2.fill(red, 0, NUM_LEDS);
    } else {
      pixels.clear();
      pixels2.clear();
    }
    pixels.show();
    pixels2.show();
    ledOn = !ledOn;
    delay(SHUTDOWN_FLASH_INTERVAL_MS);
  }

  // Phase 2: Fade from bright red to off over 5 seconds
  unsigned long fadeStart = millis();
  pixels.fill(red, 0, NUM_LEDS);
  pixels2.fill(red, 0, NUM_LEDS);

  while (millis() - fadeStart < SHUTDOWN_FADE_DURATION_MS) {
    float progress = (float)(millis() - fadeStart) / SHUTDOWN_FADE_DURATION_MS;
    uint8_t brightness = MAX_BRIGHTNESS * (1.0 - progress);
    pixels.setBrightness(brightness);
    pixels2.setBrightness(brightness);
    pixels.show();
    pixels2.show();
    delay(20);  // Smooth fade with ~50 updates per second
  }

  // Final state: LEDs off
  pixels.clear();
  pixels2.clear();
  pixels.show();
  pixels2.show();
}

void pulseLeds() {
    // Non-blocking pulsing effect
    // Uses a sine wave to smoothly ramp the brightness up and down
    float brightness = (sin(millis() / 1000.0 * PI) + 1) / 2.0;

    uint32_t color = getVisorColorValue(appState.visorColor);
    pixels.fill(color, 0, NUM_LEDS);
    pixels2.fill(color, 0, NUM_LEDS);
    pixels.setBrightness(brightness * (appState.visorBrightness * BRIGHTNESS_STEP));
    pixels2.setBrightness(brightness * (appState.visorBrightness * BRIGHTNESS_STEP));
    Serial.println("Brightness: " + String(brightness * (appState.visorBrightness * BRIGHTNESS_STEP)));
    pixels.show();
    pixels2.show();
}

void flashLeds() {
    // Non-blocking flashing effect
    // Uses millis() to toggle the LEDs on and off
    static unsigned long lastToggle = 0;
    unsigned long currentMillis = millis();

    if (currentMillis - lastToggle >= FLASH_INTERVAL_MS) {
        lastToggle = currentMillis;

        if (pixels.getBrightness() > 0) {
            pixels.setBrightness(0);
            pixels2.setBrightness(0);
        } else {
            pixels.setBrightness(appState.visorBrightness * BRIGHTNESS_STEP);
            pixels2.setBrightness(appState.visorBrightness * BRIGHTNESS_STEP);
        }

        uint32_t color = getVisorColorValue(appState.visorColor);
        pixels.fill(color, 0, NUM_LEDS);
        pixels2.fill(color, 0, NUM_LEDS);
        pixels.show();
        pixels2.show();
    }
}

void strobeLeds() {
    // Non-blocking strobe effect
    // Uses millis() to toggle the LEDs on and off rapidly
    static unsigned long lastToggle = 0;
    unsigned long currentMillis = millis();

    if (currentMillis - lastToggle >= STROBE_INTERVAL_MS) {
        lastToggle = currentMillis;

        if (pixels.getBrightness() > 0) {
            pixels.setBrightness(0);
            pixels2.setBrightness(0);
        } else {
            pixels.setBrightness(appState.visorBrightness * BRIGHTNESS_STEP);
            pixels2.setBrightness(appState.visorBrightness * BRIGHTNESS_STEP);
        }

        uint32_t color = getVisorColorValue(appState.visorColor);
        pixels.fill(color, 0, NUM_LEDS);
        pixels2.fill(color, 0, NUM_LEDS);
        pixels.show();
        pixels2.show();
    }
}

// --- Screen Saver Functions ---

void resetIdleTimer() {
    lastInteractionTime = millis();
}

void exitScreenSaver() {
    if (screenSaverActive) {
        screenSaverActive = false;
        // Force menu to redraw on next loop iteration
        if (menuController) {
            menuController->forceRedraw();
        }
    }
}

void renderScreenSaver() {
    // Placeholder implementation - renders based on appState.hudStyle
    // This will be expanded with actual animations later
    switch (appState.hudStyle) {
        case HudStyle::BIOMETRIC:
            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_GREEN);
            tft.setTextSize(2);
            tft.setCursor(80, 75);
            tft.print("BIOMETRIC");
            break;
        case HudStyle::RADAR:
            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_GREEN);
            tft.setTextSize(2);
            tft.setCursor(110, 75);
            tft.print("RADAR");
            break;
        case HudStyle::MATRIX:
            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_GREEN);
            tft.setTextSize(2);
            tft.setCursor(100, 75);
            tft.print("MATRIX");
            break;
    }
}

void saveAppState() {
    preferences.begin("spartan-state", false); // Open Preferences in read-write mode
    preferences.putBool("visorOn", appState.visorOn);
    preferences.putUChar("visorMode", (uint8_t)appState.visorMode);
    preferences.putUChar("visorColor", (uint8_t)appState.visorColor);
    preferences.putUChar("visorBrightness", appState.visorBrightness);
    preferences.putBool("thermalsOn", appState.thermalsOn);
    preferences.putUChar("hudStyle", (uint8_t)appState.hudStyle);
    preferences.putUChar("bootSequence", (uint8_t)appState.bootSequence);
    preferences.end();
}

void loadAppState() {
    preferences.begin("spartan-state", true); // Open Preferences in read-only mode
    appState.visorOn = preferences.getBool("visorOn", false); // Default to false
    appState.visorMode = (VisorMode)preferences.getUChar("visorMode", (uint8_t)VisorMode::SOLID); // Default to SOLID
    appState.visorColor = (VisorColor)preferences.getUChar("visorColor", (uint8_t)VisorColor::BLUE); // Default to BLUE
    appState.visorBrightness = preferences.getUChar("visorBrightness", 3); // Default to 3
    appState.thermalsOn = preferences.getBool("thermalsOn", false); // Default to false
    appState.hudStyle = (HudStyle)preferences.getUChar("hudStyle", (uint8_t)HudStyle::BIOMETRIC); // Default to BIOMETRIC
    appState.bootSequence = (BootSequence)preferences.getUChar("bootSequence", (uint8_t)BootSequence::UNSC_LOGO); // Default to UNSC_LOGO
    preferences.end();
}

void parseMacAddress(const char* macStr, uint8_t* macBytes) {
    // Parse MAC address string "XX:XX:XX:XX:XX:XX" into 6 bytes
    sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &macBytes[0], &macBytes[1], &macBytes[2],
           &macBytes[3], &macBytes[4], &macBytes[5]);
}

void savePeerAddresses() {
    preferences.begin("spartan-peers", false);
    preferences.putBytes("sendAddr", sendAddress, 6);
    preferences.putBytes("recvAddr", recvAddress, 6);
    preferences.end();
    Serial.println("Peer addresses saved to preferences");
}

bool loadPeerAddresses() {
    preferences.begin("spartan-peers", true);
    bool hasSendAddr = preferences.isKey("sendAddr");
    bool hasRecvAddr = preferences.isKey("recvAddr");

    if (hasSendAddr && hasRecvAddr) {
        preferences.getBytes("sendAddr", sendAddress, 6);
        preferences.getBytes("recvAddr", recvAddress, 6);
        preferences.end();

        Serial.print("Loaded peer addresses - Interface: ");
        for (int i = 0; i < 6; i++) {
            Serial.printf("%02X", sendAddress[i]);
            if (i < 5) Serial.print(":");
        }
        Serial.print(" Receiver: ");
        for (int i = 0; i < 6; i++) {
            Serial.printf("%02X", recvAddress[i]);
            if (i < 5) Serial.print(":");
        }
        Serial.println();
        return true;
    }

    preferences.end();
    Serial.println("No saved peer addresses found, using defaults");
    return false;
}