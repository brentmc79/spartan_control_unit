#include "loading_animations.h"
#include <Adafruit_ST7789.h>

int16_t dispH = 170;
int16_t dispW = 320;

/**
 * @brief Helper function to draw random noise on the screen for a glitch effect.
 * @param intensity The number of noise elements (pixels, lines) to draw.
 */
void drawNoise(int intensity, Adafruit_ST7789* displayPtr) {
  Adafruit_ST7789& display = *displayPtr;
  long screenWidth = display.width();
  long screenHeight = display.height();

  for (int i = 0; i < intensity; i++) {
    // Draw random pixels
    display.drawPixel(random(0, screenWidth), random(0, screenHeight), ST77XX_WHITE);

    // Occasionally draw random short horizontal or vertical lines for a more "blocky" glitch
    if (random(0, 10) > 8) {
      int x = random(0, screenWidth - 10);
      int y = random(0, screenHeight - 2);
      if (random(0, 2) == 0) { // Horizontal line
        display.drawFastHLine(x, y, random(5, 20), ST77XX_WHITE);
      } else { // Vertical line
        display.drawFastVLine(x, y, random(2, 10), ST77XX_WHITE);
      }
    }
  }
}

void loadingAnimation1(Adafruit_ST7789* displayPtr){
  Adafruit_ST7789& display = *displayPtr;

  for(int i=0; i<1; i++){
    for(int k=0; k<2; k++){
      if(k>0){
        display.fillScreen(ST77XX_BLACK);
        delay(700);
      }
      display.drawRect(0, 0, dispW, dispH, ST77XX_WHITE);
      display.drawLine(dispW-8, dispH-1, dispW-1, dispH-8, ST77XX_WHITE);
      display.drawTriangle(dispW-7, dispH-1, dispW-1, dispH-1, dispW-1, dispH-7, ST77XX_BLACK);
      display.setTextSize(2);           
      display.setTextColor(ST77XX_WHITE);      
      display.setCursor(86,18); 
      display.write("Mjolnir MkIV");
      display.setCursor(85,38); 
      display.write("INITIALIZING");
    }
    delay(400);
    for(int j=0; j<142; j++){
      display.drawRect(85, 56, 144, 17, ST77XX_WHITE);
      display.drawRect(86, 57, j+1, 15, ST77XX_WHITE);
      delay(25);
    }
    delay(200);
    display.fillScreen(ST77XX_BLACK);
    display.drawRect(0, 0, dispW, dispH, ST77XX_WHITE);
    display.drawLine(dispW-8, dispH-1, dispW-1, dispH-8, ST77XX_WHITE);
    display.drawTriangle(dispW-7, dispH-1, dispW-1, dispH-1, dispW-1, dispH-7, ST77XX_BLACK);
    display.setCursor(86,18);
    display.write("Mjolnir MkIV");
    display.setTextSize(5);
    display.setCursor(84,40);
    display.write("READY");
  }

  Serial.println("");
  Serial.println("Hello, XIAO ESP32-C3!");
  Serial.println("Welcome to Wokwi :-)");
}

void loadingAnimation2(Adafruit_ST7789* displayPtr){
  Adafruit_ST7789& display = *displayPtr;
  long screenWidth = display.width();
  long screenHeight = display.height();

  // --- Part 1: Intense Startup Flicker Effect ---
  // A longer, more intense sequence to simulate a rough power-on.
  for (int i = 1; i < 20; i++) {
    //display.fillScreen(ST77XX_BLACK);
    // Increase noise intensity over time
    drawNoise(i, &display);
    delay(30 + random(0, 50));
  }

  // --- Part 2: "UNSC" and Initializing Text with Flicker ---
  //display.fillScreen(ST77XX_BLACK);
  display.setTextSize(3);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(10, 10);
  display.println(F("UNSC"));
  
  display.setTextSize(2);
  display.setCursor(10, 40);
  display.println(F("Loading OS..."));
  
  display.drawRect(0, 60, screenWidth, 2, ST77XX_WHITE);
  delay(500); // Hold the initial text for a moment

  // --- Part 3: Progress Bar with Intermittent Glitches ---
  int progressBarWidth = screenWidth - 20;
  int progressBarHeight = 10;
  int progressBarX = 10;
  int progressBarY = screenHeight - progressBarHeight - 5;
  
  // Draw the progress bar outline
  display.drawRect(progressBarX - 1, progressBarY - 1, progressBarWidth + 2, progressBarHeight + 2, ST77XX_WHITE);
  
  for (int i = 0; i <= progressBarWidth; i += 6) {
    display.fillRect(progressBarX, progressBarY, i, progressBarHeight, ST77XX_WHITE);
    //display.display();

    // Add a significant glitch effect at random intervals during loading
    if (random(0, 100) > 85) { // ~15% chance of a big glitch on each step
      // Invert the display for a flash effect
      display.invertDisplay(false); 
      drawNoise(1, &display); // Draw some noise on top
      //display.display();
      delay(50);
      display.invertDisplay(true); // Revert back
    }
    
    // Add minor, constant flickering
    drawNoise(1, &display);
    delay(40);
  }
  
  // --- Part 4: Final Glitch before finishing ---
  delay(200);
  display.invertDisplay(true);
  delay(100);
  display.invertDisplay(false);
  delay(50);
  display.invertDisplay(true);
  delay(100);
  //display.invertDisplay(false);
  //delay(500);
}

void loadingAnimation3(Adafruit_ST7789* displayPtr){
  Adafruit_ST7789& display = *displayPtr;
  long screenWidth = display.width();
  long screenHeight = display.height();

  // --- Part 1: Startup Flicker Effect ---
  // Simulate a brief, unstable power-on sequence
  for (int i = 0; i < 9; i++) {
    display.fillScreen(ST77XX_BLACK);
    delay(40 + random(0, 50)); // Random delay for flicker effect
    display.drawPixel(random(0, screenWidth), random(0, screenHeight), ST77XX_WHITE); // Random pixel noise
    display.drawPixel(random(0, screenWidth), random(0, screenHeight), ST77XX_WHITE); // Random pixel noise
    delay(30 + random(0, 50));
  }

  // --- Part 2: "UNSC" and Initializing Text ---
  display.fillScreen(ST77XX_BLACK);
  display.setTextSize(2); // Larger text for UNSC
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(10, 10); // Top-left
  display.println(F("UNSC"));
  
  display.setTextSize(2); // Smaller text for "Initializing..."
  display.setCursor(10, 30); // Below UNSC
  display.println(F("Initializing..."));
  
  display.drawRect(0, 50, screenWidth, 2, ST77XX_WHITE); // Horizontal line separator
  delay(1000); // Pause to let "UNSC" and "Initializing" show

  // --- Part 3: Progress Bar Animation ---
  int progressBarWidth = screenWidth - 20; // 10 pixels padding on each side
  int progressBarHeight = 10;
  int progressBarX = 10;
  int progressBarY = screenHeight - progressBarHeight - 10; // Position near bottom, leaving space for "Loading..."
  
  // Draw the progress bar outline
  display.drawRect(progressBarX - 1, progressBarY - 1, progressBarWidth + 2, progressBarHeight + 2, ST77XX_WHITE);
  
  display.setTextSize(2);
  display.setCursor(10, progressBarY - 25); // Above the progress bar
  display.println(F("Loading"));

  for (int i = 0; i <= progressBarWidth; i += 2) { // Increment by 2 for faster fill and visual effect
    display.fillRect(progressBarX, progressBarY, i, progressBarHeight, ST77XX_WHITE);
    
    // Add some random "data stream" lines below the bar for visual interest
    if (i % 10 == 0) { // Every few steps, redraw some lines
      display.fillRect(0, progressBarY + progressBarHeight + 5, screenWidth, screenHeight - (progressBarY + progressBarHeight + 5), ST77XX_BLACK); // Clear bottom area
      for (int j = 0; j < 5; j++) {
        display.drawFastHLine(random(0, screenWidth / 2), progressBarY + progressBarHeight + 5 + (j * 3), random(10, 50), ST77XX_WHITE);
        display.drawFastHLine(random(screenWidth / 2, screenWidth), progressBarY + progressBarHeight + 5 + (j * 3), random(10, 50), ST77XX_WHITE);
      }
    }
    
    delay(20); // Adjust delay for speed of progress bar
  }
  
  delay(1000); // Keep full progress bar on screen for a moment

  // After animation, clear and show some final message
  display.fillScreen(ST77XX_BLACK);
  display.setTextSize(2);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(10,10);
  display.println(F("SYSTEM READY."));
}