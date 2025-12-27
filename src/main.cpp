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

const bool VERIFY_HARDWARE = false;
const bool UI_MODULE = true;

String macAddress;
Adafruit_ST7789 tft = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);
Adafruit_NeoPixel pixels(NUM_LEDS, LED_DATA, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel onboardLED(1, 48, NEO_GRB + NEO_KHZ800);

uint8_t sendAddress[] = {0x44, 0x17, 0x93, 0x6C, 0x62, 0x84}; // esp32 1.9 in tft lcd
uint8_t recvAddress[] = {0x24, 0x58, 0x7C, 0xDB, 0x31, 0x3C}; // esp32 s3 super mini

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

  uint8_t i = 0;
  onboardLED.setBrightness(50);
  while (i < 10) {
    onboardLED.setPixelColor(0, onboardLED.Color(0, 0, 255));
    onboardLED.show();
    delay(250);
    onboardLED.setPixelColor(0, onboardLED.Color(0, 0, 0));
    onboardLED.show();
    delay(250);
    i++;
  }
  onboardLED.clear();
  onboardLED.show();
}

void setup() {
  Serial.begin(115200);

  if(!UI_MODULE){
    onboardLED.begin();
    onboardLED.clear();
    //onboardLED.setPixelColor(0, onboardLED.Color(0, 0, 255));
    //onboardLED.setBrightness(10);
    onboardLED.show();
  }

  Serial.println("Fetching MAC address...");
  macAddress = WiFi.macAddress();
  Serial.print(macAddress);

  //pinMode(BUTTON_1, INPUT_PULLUP);
  //pinMode(BUTTON_2, INPUT_PULLUP);
  //pinMode(FAN_1_CTRL, OUTPUT);
  delay(100); // Allow pins to settle

  //pixels.begin();
  //pixels.clear();
  //pixels.show();

  if(UI_MODULE){
    tft.init(170, 320);
    tft.setRotation(1);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE);
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
  }

  if (VERIFY_HARDWARE) {
    verifyHardwareConnections(tft, pixels);
  }

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
  if (UI_MODULE)
    memcpy(peerInfo.peer_addr, recvAddress, 6);
  else
    memcpy(peerInfo.peer_addr, sendAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  //scrollLogo(&tft);
  //loadingAnimation2(&tft);
  //renderHaloLoadingAnimation(&tft);
  //loadingAnimation(&tft);

  Serial.print("Awaiting messages at ");
  Serial.println(macAddress);
}

void loop() {
  //tft.println("loop");
  if(UI_MODULE){
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
    delay(1000);
    tft.fillScreen(ST77XX_BLACK);
    delay(8000);
  } else {
    //Serial.print("Awaiting messages at ");
    //Serial.println(macAddress);
  }
  //delay(5000);
  //Serial.println("loop");
  // Your main program loop
}