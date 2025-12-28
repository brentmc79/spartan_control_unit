#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <loading_animations.h>
#include <unsc_logo.h>
#include <hardware_verification.h>
#include "pins.h"
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

const bool VERIFY_HARDWARE = false;
const bool isInterface = DEVICE_MODE == DeviceMode::INTERFACE;
const bool isInterfaceSetup = DEVICE_MODE == DeviceMode::INTERFACE_SETUP;
const bool isReceiver = DEVICE_MODE == DeviceMode::RECEIVER;
const bool isReceiverSetup = DEVICE_MODE == DeviceMode::RECEIVER_SETUP;

String macAddress;
Adafruit_ST7789 tft = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);
Adafruit_NeoPixel pixels(NUM_LEDS, LED_DATA, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel onboardLED(1, 48, NEO_GRB + NEO_KHZ800);

OneButton buttonOne = OneButton(BUTTON_1, true, true);
OneButton buttonTwo = OneButton(BUTTON_2, true, true);

uint8_t sendAddress[] = {0x44, 0x17, 0x93, 0x6C, 0x62, 0x84}; // esp32 1.9 in tft lcd
uint8_t recvAddress[] = {0x24, 0x58, 0x7C, 0xDB, 0x31, 0x3C}; // esp32 s3 super mini
uint8_t (*peerAddress)[6] = NULL;

typedef struct struct_message {
    char msg[32];
    int val;
} struct_message;

struct_message incomingReadings;
struct_message outgoingReadings;

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
}

void pingReceiver() {
  Serial.println("Clicked!");
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 10);
  tft.print("Sending message...");
  //strcpy(outgoingReadings.msg, "Hello");
  outgoingReadings.msg[0] = 'H';
  outgoingReadings.val = 123;
  Serial.print("Sending message from ");
  Serial.println(macAddress);
  esp_err_t result = esp_now_send(recvAddress, (uint8_t *) &outgoingReadings, sizeof(outgoingReadings));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
    Serial.println("ESPNOW not Init.");
  } else if (result == ESP_ERR_ESPNOW_ARG) {
    Serial.println("Invalid Argument");
  } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
    Serial.println("Internal Error");
  } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
    Serial.println("Peer not found.");
  } else {
    Serial.println("Unknown error");
  }
}

// Handler function for a single click:
static void handleClick() {
}

void setupEspComms() {
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callbacks
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // Register peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.print("Awaiting messages at ");
  Serial.println(macAddress);
}

// Execute any setup that is specific to the receiver device
void setupReceiver() {
  peerAddress = &sendAddress;

  onboardLED.begin();
  onboardLED.clear();
  onboardLED.show();

  pixels.begin();
  pixels.clear();
  pixels.show();

  pinMode(FAN_1_CTRL, OUTPUT);
  delay(100); // Allow pins to settle
}

// Execute any setup that is specific to the interface device
void setupInterface() {
  peerAddress = &recvAddress;

  tft.init(170, 320);
  tft.setRotation(1);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);

  // Single Click event attachment
  buttonOne.attachClick(handleClick);
}

void setup() {
  Serial.begin(115200);

  Serial.println("Fetching MAC address...");
  macAddress = WiFi.macAddress();
  Serial.print(macAddress);

  setupEspComms();

  if (isReceiver) {
    setupReceiver();
  }

  if (isInterface) {
    setupInterface();
  }

  if (VERIFY_HARDWARE) {
    verifyHardwareConnections(tft, pixels);
  }

  // TODO: render loading animation or scroll logo
}

void loop() {
  buttonOne.tick();
  buttonTwo.tick();
}