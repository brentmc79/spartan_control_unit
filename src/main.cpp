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
#include <theme.h>

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
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
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

  if (isInterfaceSetup) {
    tft.setCursor(0, 80);
    tft.print("Receiver MAC:");
    tft.setCursor(0, 100);
    tft.print(incomingReadings.msg);
  }
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

// Configuration for button geometry
#define BTN_HEIGHT 35
#define BTN_RADIUS 6  // Corner roundness
#define FONT_SIZE 2   // Adjust based on your specific font file

/**
 * Renders a single "Spartan" style menu item using Adafruit_GFX.
 */
void drawMenuItem(int x, int y, int w, const char* label, bool isActive) {
    uint16_t fillColor, borderColor, textColor;
    String finalLabel;

    // 1. Determine State Colors & Text
    if (isActive) {
        fillColor   = HEX_BORDER; // Electric Cyan
        borderColor = HEX_BORDER; // Electric Cyan
        textColor   = HEX_BG;     // Dark text
        finalLabel  = "> " + String(label) + " <";
    } else {
        fillColor   = HEX_MUTED;
        borderColor = HEX_MUTED;    // Dark/Muted Green
        textColor   = HEX_TEXT_PRI; // White text
        finalLabel  = String(label);
    }

    // 2. Draw the Button Body
    // Fill first to clear previous state
    tft.fillRoundRect(x+2, y+2, w-4, BTN_HEIGHT-4, BTN_RADIUS-2, fillColor);
    tft.drawRoundRect(x, y, w, BTN_HEIGHT, BTN_RADIUS, borderColor);

    if (isActive) {
      tft.drawLine(x+w-40, y+BTN_HEIGHT-3, x+w-4, y+BTN_HEIGHT-3, ST77XX_BLACK);
      tft.drawLine(x+w-39, y+BTN_HEIGHT-4, x+w-4, y+BTN_HEIGHT-4, ST77XX_BLACK);
    }

    // 3. Setup Text
    tft.setTextSize(FONT_SIZE);
    tft.setTextColor(textColor); // Background color unnecessary as we just filled the rect
    
    // 4. Calculate Centered Position (The Adafruit Way)
    int16_t  x1, y1;
    uint16_t textW, textH;

    // Measure the bounding box of the text
    // (0,0 is just a reference point for measurement)
    tft.getTextBounds(finalLabel, 0, 0, &x1, &y1, &textW, &textH);

    // Math to find the top-left cursor position that centers the text
    //int textX = x + (w - textW) / 2;
    int textX = x + 5;
    int textY = y + 1 + (BTN_HEIGHT - textH) / 2;

    // 5. Draw Text
    tft.setCursor(textX, textY);
    tft.print(finalLabel);
}

const char* menuItems[] = {"VISOR", "THERMALS", "HUD", "SETTINGS"};
const int menuCount = 4;
int selectedIndex = 2; // "HUD" is selected

void renderMenu() {
    int startX = 0;
    int startY = 5;
    int gap = 7;       // Space between buttons
    int width = 220;   // Width of the main column

    for (int i = 0; i < menuCount; i++) {
        // Calculate Y position for each item
        int currentY = startY + (i * (BTN_HEIGHT + gap));
        
        // Check if this specific item is the selected one
        bool isActive = (i == selectedIndex);
        
        drawMenuItem(startX, currentY, width, menuItems[i], isActive);
    }
}

void renderColumnDivider() {
  tft.drawLine(235, 0, 235, 165, HEX_MUTED);
  tft.drawLine(236, 0, 236, 165, HEX_BORDER);
  tft.drawLine(237, 0, 237, 165, HEX_MUTED);
  tft.drawLine(238, 0, 238, 165, HEX_MUTED);
  tft.drawLine(239, 5, 239, 170, HEX_BORDER);
  tft.drawLine(240, 5, 240, 170, HEX_MUTED);
}

void setupEspComms() {
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
  Serial.println(macAddress);
}

void setupInterfaceSetup() {
  Serial.println("Setting up interface setup");

  tft.init(170, 320);
  tft.setRotation(1);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.print("Interface MAC:");
  tft.setCursor(0, 20);
  tft.print(macAddress);

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

// Execute any setup that is specific to the receiver device
void setupReceiver() {
  Serial.println("Setting up receiver");

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
  Serial.println("Setting up interface");
  peerAddress = &recvAddress;

  tft.init(170, 320);
  tft.setRotation(1);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);

  // Single Click event attachment
  buttonOne.attachClick(handleClick);

  renderMenu();
  renderColumnDivider();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting setup");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  macAddress = WiFi.macAddress();
  Serial.print("MAC Address: ");
  Serial.println(macAddress);

  if (isInterfaceSetup) {
    setupInterfaceSetup();
    return; 
  }

  if (isReceiverSetup) {
    setupReceiverSetup();
    // No return, we need the loop to run
  }

  if (isReceiver) {
    setupReceiver();
  }

  if (isInterface) {
    setupInterface();
  }

  setupEspComms();

  if (VERIFY_HARDWARE) {
    verifyHardwareConnections(tft, pixels);
  }

  // TODO: render loading animation or scroll logo
}

void loop() {
  if (isReceiverSetup) {
    strcpy(outgoingReadings.msg, macAddress.c_str());
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
}