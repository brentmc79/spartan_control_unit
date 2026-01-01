#include "screensavers.h"
#include "layout.h"
#include "spartan_image.h"
#include <Arduino.h>

// --- Matrix Screen Saver ---

// Character size for Matrix effect (using font size 1)
#define MATRIX_CHAR_WIDTH 6
#define MATRIX_CHAR_HEIGHT 10
#define MATRIX_COLUMNS (SCREEN_WIDTH / MATRIX_CHAR_WIDTH)
#define MATRIX_ROWS (SCREEN_HEIGHT / MATRIX_CHAR_HEIGHT)

// Matrix colors - bright head fading to dark green trail
#define MATRIX_HEAD_COLOR 0xFFFF      // White
#define MATRIX_BRIGHT_GREEN 0x07E0    // Bright green
#define MATRIX_MID_GREEN 0x03E0       // Medium green
#define MATRIX_DIM_GREEN 0x01E0       // Dim green
#define MATRIX_DARK_GREEN 0x00E0      // Dark green

// Stream state for each column
struct MatrixStream {
    int16_t headY;          // Current Y position of the stream head (in character rows)
    int8_t speed;           // How many frames between moves (lower = faster)
    int8_t frameCount;      // Frame counter for speed control
    int8_t trailLength;     // Length of the visible trail
    bool active;            // Whether this stream is currently falling
};

static MatrixStream streams[MATRIX_COLUMNS];
static char matrixChars[MATRIX_COLUMNS][MATRIX_ROWS];  // Character buffer
static bool matrixInitialized = false;

// Get a random Matrix-style character
static char getRandomMatrixChar() {
    // Mix of numbers, letters, and symbols for that Matrix look
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*(){}[]|;:<>?";
    return charset[random(sizeof(charset) - 1)];
}

// Get color based on distance from head (0 = head, higher = further back)
static uint16_t getTrailColor(int distance) {
    if (distance == 0) return MATRIX_HEAD_COLOR;
    if (distance == 1) return MATRIX_BRIGHT_GREEN;
    if (distance <= 3) return MATRIX_MID_GREEN;
    if (distance <= 6) return MATRIX_DIM_GREEN;
    return MATRIX_DARK_GREEN;
}

void initMatrixScreenSaver() {
    // Initialize all streams
    for (int i = 0; i < MATRIX_COLUMNS; i++) {
        streams[i].headY = random(-MATRIX_ROWS, 0);  // Start above screen
        streams[i].speed = random(20, 50);              // Variable speeds (higher = slower)
        streams[i].frameCount = 0;
        streams[i].trailLength = random(5, MATRIX_ROWS - 2);
        streams[i].active = true;

        // Initialize characters for this column
        for (int j = 0; j < MATRIX_ROWS; j++) {
            matrixChars[i][j] = getRandomMatrixChar();
        }
    }
    matrixInitialized = true;
}

void renderMatrixScreenSaver(TFT_eSPI& tft) {
    if (!matrixInitialized) {
        initMatrixScreenSaver();
        tft.fillScreen(TFT_BLACK);
    }

    tft.startWrite();

    for (int col = 0; col < MATRIX_COLUMNS; col++) {
        MatrixStream& stream = streams[col];

        // Update frame counter
        stream.frameCount++;
        if (stream.frameCount < stream.speed) {
            continue;  // Not time to update this stream yet
        }
        stream.frameCount = 0;

        // Calculate pixel X position for this column
        int16_t pixelX = col * MATRIX_CHAR_WIDTH;

        // Erase the character at the tail end (where trail ends)
        int tailRow = stream.headY - stream.trailLength;
        if (tailRow >= 0 && tailRow < MATRIX_ROWS) {
            int16_t pixelY = tailRow * MATRIX_CHAR_HEIGHT;
            tft.fillRect(pixelX, pixelY, MATRIX_CHAR_WIDTH, MATRIX_CHAR_HEIGHT, TFT_BLACK);
        }

        // Move the stream down
        stream.headY++;

        // Draw the visible portion of the stream
        for (int i = 0; i <= stream.trailLength; i++) {
            int row = stream.headY - i;
            if (row >= 0 && row < MATRIX_ROWS) {
                int16_t pixelY = row * MATRIX_CHAR_HEIGHT;
                uint16_t color = getTrailColor(i);

                // Randomly change character occasionally (adds flicker effect)
                if (random(10) == 0) {
                    matrixChars[col][row] = getRandomMatrixChar();
                }

                // Draw character
                tft.setTextColor(color, TFT_BLACK);
                tft.setTextSize(1);
                tft.setCursor(pixelX, pixelY);
                tft.print(matrixChars[col][row]);
            }
        }

        // Reset stream if it's completely off screen
        if (stream.headY - stream.trailLength >= MATRIX_ROWS) {
            stream.headY = random(-10, -1);  // Reset above screen
            stream.speed = random(20, 50);     // Variable speeds (higher = slower)
            stream.trailLength = random(5, MATRIX_ROWS - 2);

            // Refresh characters for this column
            for (int j = 0; j < MATRIX_ROWS; j++) {
                matrixChars[col][j] = getRandomMatrixChar();
            }
        }
    }

    tft.endWrite();
}

// --- Biometric Screen Saver ---

// Biometric display colors (cyan/teal HUD theme)
#define BIO_PRIMARY 0x07FF      // Cyan
#define BIO_SECONDARY 0x0410    // Dark cyan
#define BIO_DIM 0x0208          // Very dim cyan
#define BIO_ACCENT 0x07EF       // Slightly different cyan for variety

// Layout constants
#define BIO_ECG_X 10
#define BIO_ECG_Y 30
#define BIO_ECG_WIDTH 95
#define BIO_ECG_HEIGHT 60

#define BIO_DNA_X 115
#define BIO_DNA_Y 25
#define BIO_DNA_WIDTH 70
#define BIO_DNA_HEIGHT 110

#define BIO_BODY_X 200
#define BIO_BODY_Y 15
#define BIO_BODY_WIDTH 110
#define BIO_BODY_HEIGHT 130

// ECG waveform pattern (one heartbeat cycle)
// Values represent Y offset from baseline (negative = up)
static const int8_t ecgPattern[] = {
    0, 0, 0, 0, 0,           // Flat baseline
    0, -2, -4, -2, 0,        // Small P wave
    0, 0, 0,                 // Short pause
    2, 5, -25, 20, -5, 0,    // QRS complex (the main spike)
    0, 0, 0, 0,              // Short pause
    0, -3, -6, -6, -4, -2, 0, // T wave
    0, 0, 0, 0, 0, 0         // Return to baseline
};
#define ECG_PATTERN_LEN (sizeof(ecgPattern) / sizeof(ecgPattern[0]))

// Biometric state
struct BiometricState {
    int ecgPosition;         // Current scroll position in ECG
    int ecgBuffer[100];      // Circular buffer for ECG display
    int ecgWritePos;         // Write position in buffer
    float dnaPhase;          // DNA helix rotation phase
    int heartRate;           // Displayed heart rate
    int oxygenSat;           // Displayed oxygen saturation
    unsigned long lastUpdate;
    unsigned long lastValueChange;
    bool initialized;
};

static BiometricState bioState;

// Draw ECG panel with animated waveform
static void drawECGPanel(TFT_eSPI& tft) {
    // Draw panel border
    tft.drawRect(BIO_ECG_X - 2, BIO_ECG_Y - 2, BIO_ECG_WIDTH + 4, BIO_ECG_HEIGHT + 4, BIO_SECONDARY);

    // Clear ECG area
    tft.fillRect(BIO_ECG_X, BIO_ECG_Y, BIO_ECG_WIDTH, BIO_ECG_HEIGHT, TFT_BLACK);

    // Draw ECG grid (subtle)
    for (int x = BIO_ECG_X; x < BIO_ECG_X + BIO_ECG_WIDTH; x += 10) {
        tft.drawFastVLine(x, BIO_ECG_Y, BIO_ECG_HEIGHT, BIO_DIM);
    }
    for (int y = BIO_ECG_Y; y < BIO_ECG_Y + BIO_ECG_HEIGHT; y += 10) {
        tft.drawFastHLine(BIO_ECG_X, y, BIO_ECG_WIDTH, BIO_DIM);
    }

    // Draw ECG waveform from buffer
    int baseline = BIO_ECG_Y + BIO_ECG_HEIGHT / 2;

    for (int i = 0; i < BIO_ECG_WIDTH - 1; i++) {
        int bufIdx = (bioState.ecgWritePos + i) % BIO_ECG_WIDTH;
        int nextBufIdx = (bioState.ecgWritePos + i + 1) % BIO_ECG_WIDTH;

        int y1 = baseline + bioState.ecgBuffer[bufIdx];
        int y2 = baseline + bioState.ecgBuffer[nextBufIdx];

        // Clamp to panel bounds
        y1 = constrain(y1, BIO_ECG_Y + 2, BIO_ECG_Y + BIO_ECG_HEIGHT - 2);
        y2 = constrain(y2, BIO_ECG_Y + 2, BIO_ECG_Y + BIO_ECG_HEIGHT - 2);

        tft.drawLine(BIO_ECG_X + i, y1, BIO_ECG_X + i + 1, y2, BIO_PRIMARY);
    }

    // Draw heart icon (small)
    int heartX = BIO_ECG_X + 5;
    int heartY = BIO_ECG_Y + BIO_ECG_HEIGHT + 8;
    tft.fillCircle(heartX + 3, heartY + 2, 3, BIO_PRIMARY);
    tft.fillCircle(heartX + 9, heartY + 2, 3, BIO_PRIMARY);
    tft.fillTriangle(heartX, heartY + 3, heartX + 12, heartY + 3, heartX + 6, heartY + 10, BIO_PRIMARY);
}

// Draw DNA helix animation
static void drawDNAHelix(TFT_eSPI& tft) {
    // Clear DNA area
    tft.fillRect(BIO_DNA_X, BIO_DNA_Y, BIO_DNA_WIDTH, BIO_DNA_HEIGHT, TFT_BLACK);

    // Draw panel border
    tft.drawRect(BIO_DNA_X - 2, BIO_DNA_Y - 2, BIO_DNA_WIDTH + 4, BIO_DNA_HEIGHT + 4, BIO_SECONDARY);

    int centerX = BIO_DNA_X + BIO_DNA_WIDTH / 2;
    int amplitude = 25;

    // Draw the two strands and connecting bars
    for (int y = 0; y < BIO_DNA_HEIGHT; y += 3) {
        float angle = bioState.dnaPhase + (y * 0.08f);

        int x1 = centerX + (int)(sin(angle) * amplitude);
        int x2 = centerX + (int)(sin(angle + 3.14159f) * amplitude);

        int screenY = BIO_DNA_Y + y;

        // Determine which strand is in front based on cosine
        float depth = cos(angle);

        // Draw connecting bars (rungs) every few pixels
        if (y % 9 == 0) {
            // Color based on depth for 3D effect
            uint16_t barColor = (depth > 0) ? BIO_PRIMARY : BIO_SECONDARY;
            tft.drawLine(x1, screenY, x2, screenY, barColor);
        }

        // Draw strand points with depth-based brightness
        if (depth > 0) {
            tft.drawPixel(x1, screenY, BIO_PRIMARY);
            tft.drawPixel(x1, screenY + 1, BIO_PRIMARY);
            tft.drawPixel(x2, screenY, BIO_SECONDARY);
        } else {
            tft.drawPixel(x2, screenY, BIO_PRIMARY);
            tft.drawPixel(x2, screenY + 1, BIO_PRIMARY);
            tft.drawPixel(x1, screenY, BIO_SECONDARY);
        }
    }
}

// Draw Spartan armor image
static void drawSpartanImage(TFT_eSPI& tft) {
    // Center the image in the body panel area
    int x = BIO_BODY_X + (BIO_BODY_WIDTH - SPARTAN_IMAGE_WIDTH) / 2;
    int y = BIO_BODY_Y + (BIO_BODY_HEIGHT - SPARTAN_IMAGE_HEIGHT) / 2;

    // Draw the image from PROGMEM
    tft.pushImage(x, y, SPARTAN_IMAGE_WIDTH, SPARTAN_IMAGE_HEIGHT, spartan_image);
}

// Draw text labels and values
static void drawBioLabels(TFT_eSPI& tft) {
    // Heart rate label (below ECG)
    tft.setTextColor(BIO_PRIMARY, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(BIO_ECG_X + 20, BIO_ECG_Y + BIO_ECG_HEIGHT + 10);
    tft.print("HEART RATE:");

    // Heart rate value
    tft.setCursor(BIO_ECG_X + 20, BIO_ECG_Y + BIO_ECG_HEIGHT + 22);
    char hrStr[10];
    snprintf(hrStr, sizeof(hrStr), "%d BPM", bioState.heartRate);
    tft.print(hrStr);

    // Oxygen saturation (below DNA)
    tft.setCursor(BIO_DNA_X + 5, BIO_DNA_Y + BIO_DNA_HEIGHT + 10);
    tft.print("OXYGEN SAT:");

    tft.setCursor(BIO_DNA_X + 5, BIO_DNA_Y + BIO_DNA_HEIGHT + 22);
    char o2Str[10];
    snprintf(o2Str, sizeof(o2Str), "%d%%", bioState.oxygenSat);
    tft.print(o2Str);

    // Status label (below body)
    tft.setCursor(BIO_BODY_X + 25, BIO_BODY_Y + BIO_BODY_HEIGHT + 5);
    tft.print("STATUS: OK");
}

void initBiometricScreenSaver() {
    bioState.ecgPosition = 0;
    bioState.ecgWritePos = 0;
    bioState.dnaPhase = 0;
    bioState.heartRate = 72;
    bioState.oxygenSat = 98;
    bioState.lastUpdate = 0;
    bioState.lastValueChange = 0;
    bioState.initialized = false;

    // Initialize ECG buffer with baseline
    for (int i = 0; i < 100; i++) {
        bioState.ecgBuffer[i] = 0;
    }
}

void renderBiometricScreenSaver(TFT_eSPI& tft) {
    unsigned long now = millis();

    // First-time initialization
    if (!bioState.initialized) {
        initBiometricScreenSaver();
        tft.fillScreen(TFT_BLACK);
        drawSpartanImage(tft);  // Static element, draw once
        bioState.initialized = true;
        bioState.lastUpdate = now;
        bioState.lastValueChange = now;
    }

    // Update animations at ~30fps (every 33ms)
    if (now - bioState.lastUpdate < 33) {
        return;
    }
    bioState.lastUpdate = now;

    tft.startWrite();

    // Update ECG buffer with next sample
    bioState.ecgBuffer[bioState.ecgWritePos] = ecgPattern[bioState.ecgPosition];
    bioState.ecgWritePos = (bioState.ecgWritePos + 1) % BIO_ECG_WIDTH;
    bioState.ecgPosition = (bioState.ecgPosition + 1) % ECG_PATTERN_LEN;

    // Update DNA phase
    bioState.dnaPhase += 0.08f;
    if (bioState.dnaPhase > 6.28318f) {
        bioState.dnaPhase -= 6.28318f;
    }

    // Occasionally vary the biometric values for realism
    if (now - bioState.lastValueChange > 3000) {
        bioState.heartRate = 68 + random(10);     // 68-77 BPM
        bioState.oxygenSat = 96 + random(4);      // 96-99%
        bioState.lastValueChange = now;
    }

    // Render animated elements
    drawECGPanel(tft);
    drawDNAHelix(tft);
    drawBioLabels(tft);

    tft.endWrite();
}

// --- Radar Screen Saver (placeholder) ---

void initRadarScreenSaver() {
    // TODO: Initialize radar animation state
}

void renderRadarScreenSaver(TFT_eSPI& tft) {
    // Placeholder - just show text for now
    static bool initialized = false;
    if (!initialized) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN);
        tft.setTextSize(2);
        tft.setCursor(110, 75);
        tft.print("RADAR");
        initialized = true;
    }
}
