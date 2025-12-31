#include "screensavers.h"
#include "layout.h"
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

// --- Biometric Screen Saver (placeholder) ---

void initBiometricScreenSaver() {
    // TODO: Initialize biometric animation state
}

void renderBiometricScreenSaver(TFT_eSPI& tft) {
    // Placeholder - just show text for now
    static bool initialized = false;
    if (!initialized) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN);
        tft.setTextSize(2);
        tft.setCursor(80, 75);
        tft.print("BIOMETRIC");
        initialized = true;
    }
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
