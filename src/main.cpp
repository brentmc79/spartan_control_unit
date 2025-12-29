#include <Arduino.h>
#include <TFT_eSPI.h>
#include "pins.h"
#include "theme.h"
#include "menu_system.h"
#include "communication.h"
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <esp_now.h>
#include <OneButton.h>

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
Adafruit_NeoPixel onboardLED(1, 48, NEO_GRB + NEO_KHZ800);
OneButton buttonOne(BUTTON_1, true, true);
OneButton buttonTwo(BUTTON_2, true, true);

MenuController* menuController = nullptr;

// Defined in menu_system.cpp
extern MenuItem mainMenuItems[];
extern const int mainMenuItemCount;

uint8_t sendAddress[] = {0x44, 0x17, 0x93, 0x6C, 0x62, 0x84}; // esp32 1.9 in tft lcd
uint8_t recvAddress[] = {0x24, 0x58, 0x7C, 0xDB, 0x31, 0x3C}; // esp32 s3 super mini
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t (*peerAddress)[6] = NULL;

typedef struct struct_message {
    char msg[32];
    int val;
} struct_message;

struct_message incomingReadings;
struct_message outgoingReadings;

// --- Forward Declarations ---
void handleNext();
void handleSelect();
void setupInterface();
void setupReceiver();
void setupInterfaceSetup();
void setupReceiverSetup();
void setupEspComms();
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);


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

void setupReceiver() {
  Serial.println("Setting up receiver");

  pixels.begin();
  pixels.clear();
  pixels.show();

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
  if (isInterface) {
    peerAddress = &recvAddress;
  } else {
    peerAddress = &sendAddress;
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

  // Register peer
  Serial.println("Registering peer");
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerAddress, 6);
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

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

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
      strcpy(outgoingReadings.msg, WiFi.macAddress().c_str());
      esp_now_send(broadcastAddress, (uint8_t *) &outgoingReadings, sizeof(outgoingReadings));
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

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Message from Peer: ");
  Serial.println(incomingReadings.msg);
  Serial.print("Value from Peer: ");
  Serial.println(incomingReadings.val);

  if (isInterfaceSetup) {
    tft.setCursor(0, 80);
    tft.print("Receiver MAC:");
    tft.setCursor(0, 100);
    tft.print(incomingReadings.msg);
  } else if (isReceiver) { // Blink onboard LED on receiver for incoming message
    onboardLED.setPixelColor(0, onboardLED.Color(0, 0, 255)); // Blue color
    onboardLED.setBrightness(10);
    onboardLED.show();
    delay(50);
    onboardLED.clear();
    onboardLED.show();
  }
}