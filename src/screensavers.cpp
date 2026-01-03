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
#define BIO_ECG_X 20
#define BIO_ECG_Y 30
#define BIO_ECG_WIDTH 95
#define BIO_ECG_HEIGHT 60

#define BIO_DNA_X 230
#define BIO_DNA_Y 20
#define BIO_DNA_WIDTH 70
#define BIO_DNA_HEIGHT 110

#define BIO_BODY_X 125
#define BIO_BODY_Y 10
#define BIO_BODY_WIDTH 90
#define BIO_BODY_HEIGHT 150

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
    int bodyImage;
};

static BiometricState bioState;

// Draw ECG panel with animated waveform
static void drawECGPanel(TFT_eSPI& tft) {
    // Draw panel border
    // tft.drawRoundRect(BIO_ECG_X - 5, BIO_ECG_Y - 5, BIO_ECG_WIDTH + 20, BIO_ECG_HEIGHT + 44, 4, BIO_SECONDARY);

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
    //tft.drawRect(BIO_DNA_X - 2, BIO_DNA_Y - 2, BIO_DNA_WIDTH + 4, BIO_DNA_HEIGHT + 4, BIO_SECONDARY);

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
    // Draw panel border
    if(bioState.bodyImage == 0)
        tft.drawRoundRect(BIO_BODY_X - 4, BIO_BODY_Y - 4, BIO_BODY_WIDTH + 8, BIO_BODY_HEIGHT + 8, 4, BIO_SECONDARY);
    else
        tft.drawRoundRect(BIO_BODY_X - 4, BIO_BODY_Y - 4, BIO_BODY_WIDTH + 8, BIO_BODY_HEIGHT + 8, 4, TFT_RED);

    // Center the image in the body panel area
    int x = BIO_BODY_X + (BIO_BODY_WIDTH - SPARTAN_IMAGE_WIDTH) / 2;
    int y = BIO_BODY_Y + (BIO_BODY_HEIGHT - SPARTAN_IMAGE_HEIGHT) / 2;

    // Draw the image from PROGMEM
    tft.pushImage(x, y, SPARTAN_IMAGE_WIDTH, SPARTAN_IMAGE_HEIGHT, spartan_bitmaps[bioState.bodyImage]);
}

// Draw text labels and values
static void drawBioLabels(TFT_eSPI& tft) {
    // Heart rate label (below ECG)
    tft.setTextColor(BIO_PRIMARY, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(BIO_ECG_X + 25, BIO_ECG_Y + BIO_ECG_HEIGHT + 10);
    tft.print("RATE:");

    // Heart rate value
    tft.setCursor(BIO_ECG_X + 55, BIO_ECG_Y + BIO_ECG_HEIGHT + 10);
    char hrStr[10];
    snprintf(hrStr, sizeof(hrStr), "%d BPM", bioState.heartRate);
    tft.print(hrStr);

    // Oxygen saturation (below DNA)
    tft.setCursor(BIO_ECG_X + 6, BIO_ECG_Y + BIO_ECG_HEIGHT + 25);
    tft.print("OXY SAT:");

    tft.setCursor(BIO_ECG_X + 56, BIO_ECG_Y + BIO_ECG_HEIGHT + 25);
    char o2Str[10];
    snprintf(o2Str, sizeof(o2Str), "%d%%", bioState.oxygenSat);
    tft.print(o2Str);

    tft.setCursor(BIO_DNA_X + 5, BIO_DNA_Y + BIO_DNA_HEIGHT + 10);
    tft.print("DNA ANALYSIS");
}

void initBiometricScreenSaver() {
    bioState.ecgPosition = 0;
    bioState.ecgWritePos = 0;
    bioState.dnaPhase = 0;
    bioState.heartRate = 72;
    bioState.oxygenSat = 98;
    bioState.lastUpdate = 0;
    bioState.lastValueChange = 0;
    bioState.bodyImage = 0;
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
        bioState.bodyImage = random(spartan_bitmaps_LEN);
        bioState.lastValueChange = now;
    }

    // Render animated elements
    drawECGPanel(tft);
    drawDNAHelix(tft);
    drawBioLabels(tft);
    drawSpartanImage(tft);

    tft.endWrite();
}

// --- Radar Screen Saver ---

// Radar display colors (using cyan theme to match biometric screen saver)
#define RADAR_PRIMARY BIO_PRIMARY       // Cyan (0x07FF)
#define RADAR_DIM BIO_SECONDARY         // Dark cyan (0x0410)
#define RADAR_VERY_DIM BIO_DIM          // Very dim cyan (0x0208)
#define RADAR_SWEEP BIO_PRIMARY         // Bright cyan for sweep line
#define RADAR_TARGET 0xFFE0             // Yellow for targets
#define RADAR_TARGET_PULSE 0xFFFF       // White when pulsing

// Radar layout constants (screen coordinates)
#define RADAR_LEFT_BORDER 57
#define RADAR_RIGHT_BORDER 263
#define RADAR_SPRITE_WIDTH (RADAR_RIGHT_BORDER - RADAR_LEFT_BORDER)
#define RADAR_SPRITE_HEIGHT SCREEN_HEIGHT

// Radar layout constants (sprite-local coordinates)
#define RADAR_CENTER_X_LOCAL (RADAR_SPRITE_WIDTH / 2)   // Center X within sprite
#define RADAR_CENTER_Y_LOCAL (RADAR_SPRITE_HEIGHT - 5)  // 5px above sprite bottom
#define RADAR_RADIUS (RADAR_SPRITE_HEIGHT - 5)          // Extends to sprite top
#define RADAR_INNER_RADIUS 20

// Target constants
#define MAX_TARGETS 3
#define TARGET_SPAWN_CHANCE 150   // 1 in N chance per frame to spawn

// Sweep parameters
#define SWEEP_SPEED 1.5f          // Degrees per frame
#define SWEEP_TRAIL_ANGLE 15.0f   // Degrees of fade trail behind sweep

// Radar sprite (created once, reused each frame)
static TFT_eSprite* radarSprite = nullptr;

// Target structure
struct RadarTarget {
    float angle;          // Current angle in degrees (0-180)
    float distance;       // Distance from center (0.0 to 1.0 normalized)
    float speed;          // Movement speed (angle change per frame)
    float distSpeed;      // Radial movement speed
    bool active;          // Whether target is visible
    bool pulsing;         // Whether currently intersected by sweep
    uint8_t pulseCount;   // Pulse animation counter
    int direction;        // 1 = moving right, -1 = moving left
};

// Radar state
struct RadarState {
    float sweepAngle;           // Current sweep angle (0-180)
    int sweepDirection;         // 1 = sweeping right, -1 = sweeping left
    RadarTarget targets[MAX_TARGETS];
    unsigned long lastUpdate;
    unsigned long lastDataChange;
    bool initialized;
    // Alphanumeric data
    int contactCount;
    int scanCycle;
    float signalStrength;
    int bearing;
};

static RadarState radarState;

// Convert degrees to radians
static float degToRad(float deg) {
    return deg * 3.14159f / 180.0f;
}

// Clip a line from center to (x2,y2) to sprite bounds, returning the clipped endpoint
static void clipLineToSprite(int cx, int cy, int& x2, int& y2) {
    // Calculate direction vector
    float dx = x2 - cx;
    float dy = y2 - cy;

    // Find the smallest t value where line intersects a boundary
    float t = 1.0f;

    // Check right boundary
    if (dx > 0 && x2 >= RADAR_SPRITE_WIDTH) {
        float t_right = (RADAR_SPRITE_WIDTH - 1 - cx) / dx;
        if (t_right < t) t = t_right;
    }
    // Check left boundary
    if (dx < 0 && x2 < 0) {
        float t_left = -cx / dx;
        if (t_left < t) t = t_left;
    }
    // Check top boundary
    if (dy < 0 && y2 < 0) {
        float t_top = -cy / dy;
        if (t_top < t) t = t_top;
    }
    // Check bottom boundary
    if (dy > 0 && y2 >= RADAR_SPRITE_HEIGHT) {
        float t_bottom = (RADAR_SPRITE_HEIGHT - 1 - cy) / dy;
        if (t_bottom < t) t = t_bottom;
    }

    // Apply clipping
    if (t < 1.0f) {
        x2 = cx + (int)(dx * t);
        y2 = cy + (int)(dy * t);
    }
}

// Draw radar arcs (range rings) - draws to sprite with local coordinates
static void drawRadarArcs(TFT_eSprite& spr) {
    // Draw concentric arcs for range indication
    for (int r = RADAR_INNER_RADIUS; r <= RADAR_RADIUS; r += 25) {
        // Draw arc using line segments (semi-circle, 180 degrees)
        for (int deg = 0; deg < 180; deg += 3) {
            float rad = degToRad(deg);
            int x = RADAR_CENTER_X_LOCAL + (int)(cos(rad) * r);
            int y = RADAR_CENTER_Y_LOCAL - (int)(sin(rad) * r);
            if (y >= 0 && y < RADAR_SPRITE_HEIGHT && x >= 0 && x < RADAR_SPRITE_WIDTH) {
                spr.drawPixel(x, y, RADAR_VERY_DIM);
            }
        }
    }

    // Draw radial lines at 45 degree intervals
    for (int deg = 0; deg <= 180; deg += 45) {
        float rad = degToRad(deg);
        int x2 = RADAR_CENTER_X_LOCAL + (int)(cos(rad) * RADAR_RADIUS);
        int y2 = RADAR_CENTER_Y_LOCAL - (int)(sin(rad) * RADAR_RADIUS);
        clipLineToSprite(RADAR_CENTER_X_LOCAL, RADAR_CENTER_Y_LOCAL, x2, y2);
        spr.drawLine(RADAR_CENTER_X_LOCAL, RADAR_CENTER_Y_LOCAL, x2, y2, RADAR_VERY_DIM);
    }

    // Draw baseline (flat edge of semi-circle)
    spr.drawFastHLine(0, RADAR_CENTER_Y_LOCAL, RADAR_SPRITE_WIDTH, RADAR_DIM);
}

// Draw sweep line with fade trail - draws to sprite with local coordinates
static void drawSweepLine(TFT_eSprite& spr) {
    float sweepRad = degToRad(radarState.sweepAngle);

    // Draw main sweep line
    int x2 = RADAR_CENTER_X_LOCAL + (int)(cos(sweepRad) * RADAR_RADIUS);
    int y2 = RADAR_CENTER_Y_LOCAL - (int)(sin(sweepRad) * RADAR_RADIUS);
    clipLineToSprite(RADAR_CENTER_X_LOCAL, RADAR_CENTER_Y_LOCAL, x2, y2);
    spr.drawLine(RADAR_CENTER_X_LOCAL, RADAR_CENTER_Y_LOCAL, x2, y2, RADAR_SWEEP);

    // Draw fade trail behind sweep
    for (int i = 1; i <= 4; i++) {
        float trailAngle = radarState.sweepAngle - (radarState.sweepDirection * i * 6.0f);

        if (trailAngle > 0 && trailAngle < 180) {
            float trailRad = degToRad(trailAngle);
            int tx = RADAR_CENTER_X_LOCAL + (int)(cos(trailRad) * RADAR_RADIUS);
            int ty = RADAR_CENTER_Y_LOCAL - (int)(sin(trailRad) * RADAR_RADIUS);
            clipLineToSprite(RADAR_CENTER_X_LOCAL, RADAR_CENTER_Y_LOCAL, tx, ty);

            // Fade color based on distance from sweep
            uint16_t fadeColor = (i <= 2) ? RADAR_DIM : RADAR_VERY_DIM;
            spr.drawLine(RADAR_CENTER_X_LOCAL, RADAR_CENTER_Y_LOCAL, tx, ty, fadeColor);
        }
    }
}

// Draw targets on radar - draws to sprite with local coordinates
static void drawTargets(TFT_eSprite& spr) {
    for (int i = 0; i < MAX_TARGETS; i++) {
        RadarTarget& target = radarState.targets[i];
        if (!target.active) continue;

        float rad = degToRad(target.angle);
        int dist = RADAR_INNER_RADIUS + (int)(target.distance * (RADAR_RADIUS - RADAR_INNER_RADIUS));
        int x = RADAR_CENTER_X_LOCAL + (int)(cos(rad) * dist);
        int y = RADAR_CENTER_Y_LOCAL - (int)(sin(rad) * dist);

        // Check bounds (sprite-local)
        if (y < 0 || y >= RADAR_SPRITE_HEIGHT) continue;

        // Determine color based on pulse state
        uint16_t color;
        if (target.pulsing && (target.pulseCount % 4 < 2)) {
            color = RADAR_TARGET_PULSE;
        } else {
            color = RADAR_TARGET;
        }

        // Draw target blip (small cross/diamond)
        if (x > 2 && x < RADAR_SPRITE_WIDTH - 2) {
            spr.drawPixel(x, y, color);
            spr.drawPixel(x - 1, y, color);
            spr.drawPixel(x + 1, y, color);
            spr.drawPixel(x, y - 1, color);
            spr.drawPixel(x, y + 1, color);

            // Larger indicator when pulsing
            if (target.pulsing) {
                spr.drawPixel(x - 2, y, color);
                spr.drawPixel(x + 2, y, color);
                spr.drawPixel(x, y - 2, color);
                spr.drawPixel(x, y + 2, color);
            }
        }
    }
}

// Draw borders on radar - draws to sprite with local coordinates
static void drawBorders(TFT_eSprite& spr) {
    spr.drawRect(0, 0, RADAR_SPRITE_WIDTH, RADAR_SPRITE_HEIGHT, TFT_BLACK);
    spr.drawRoundRect(1, 1, RADAR_SPRITE_WIDTH-1, RADAR_SPRITE_HEIGHT-1, 6, RADAR_DIM);
    spr.drawRect(1, 1, RADAR_SPRITE_WIDTH-2, RADAR_SPRITE_HEIGHT-2, TFT_BLACK);
    spr.drawRect(2, 2, RADAR_SPRITE_WIDTH-4, RADAR_SPRITE_HEIGHT-4, TFT_BLACK);
    spr.drawRoundRect(1, 1, RADAR_SPRITE_WIDTH-2, RADAR_SPRITE_HEIGHT-2, 4, RADAR_PRIMARY);
}

// Draw left panel with alphanumeric data (draws directly to TFT)
static void drawLeftPanel(TFT_eSPI& tft) {
    int panelY = 20;
    int cursorY = panelY + 10;
    int textHeight = 10;

    // Draw border with top/left/bottom edges, mirrored from right panel
    int leftX = 2;
    int rightX = RADAR_LEFT_BORDER - 2;
    int topY = panelY;
    int bottomY = panelY + 119;
    int radius = 4;

    // Top horizontal line
    tft.drawFastHLine(leftX + radius, topY, rightX - leftX - radius, RADAR_DIM);
    // Top arc (cornername 1 = top-right quadrant, mirrored from right panel)
    tft.drawCircleHelper(leftX + radius, topY + radius, radius, 1, RADAR_DIM);
    // Left vertical line (between the two arcs)
    tft.drawFastVLine(leftX, topY + radius, bottomY - topY - 2 * radius + 1, RADAR_DIM);
    // Bottom arc (cornername 8 = top-left quadrant, mirrored from right panel)
    tft.drawCircleHelper(leftX + radius, bottomY - radius, radius, 8, RADAR_DIM);
    // Bottom horizontal line
    tft.drawFastHLine(leftX + radius, bottomY, rightX - leftX - radius, RADAR_DIM);

    int panelX = 7;  // Text inset from left edge

    tft.setTextColor(RADAR_DIM, TFT_BLACK);
    tft.setTextSize(1);

    // Title
    tft.setCursor(panelX, cursorY);
    tft.print("TACTICAL");

    // Scan cycle
    tft.setCursor(panelX, cursorY += (textHeight + 10));
    tft.print("SCAN:");
    char scanStr[8];
    snprintf(scanStr, sizeof(scanStr), "%04d", radarState.scanCycle);
    tft.setCursor(panelX, cursorY += textHeight);
    tft.print(scanStr);

    // Target count
    tft.setCursor(panelX, cursorY += textHeight + 10);
    tft.print("TARGETS");
    tft.setCursor(panelX, cursorY += textHeight);
    char contactStr[4];
    snprintf(contactStr, sizeof(contactStr), "%d", radarState.contactCount);
    tft.print(contactStr);

    // Mode indicator
    tft.setCursor(panelX, cursorY += textHeight + 10);
    tft.print("MODE:");
    tft.setCursor(panelX, cursorY += textHeight);
    tft.print("SWEEP");
}

// Draw right panel with alphanumeric data (draws directly to TFT)
static void drawRightPanel(TFT_eSPI& tft) {
    int panelX = RADAR_RIGHT_BORDER + 5;
    int panelY = 20;
    int cursorY = panelY + 10;
    int textHeight = 10;

    // Draw border with top/right/bottom edges, arcs rotated 90 degrees
    int leftX = RADAR_RIGHT_BORDER + 2;
    int rightX = SCREEN_WIDTH - 3;
    int topY = panelY;
    int bottomY = panelY + 119;
    int radius = 4;

    // Top horizontal line
    tft.drawFastHLine(leftX, topY, rightX - leftX - radius + 1, RADAR_DIM);
    // Top arc (cornername 8 = top-left quadrant, rotated 90° from top-right)
    tft.drawCircleHelper(rightX - radius, topY + radius, radius, 2, RADAR_DIM);
    // Right vertical line (between the two arcs)
    tft.drawFastVLine(rightX, topY + radius, bottomY - topY - 2 * radius + 1, RADAR_DIM);
    // Bottom arc (cornername 4 = bottom-left quadrant, rotated 90° from bottom-right)
    tft.drawCircleHelper(rightX - radius, bottomY - radius, radius, 4, RADAR_DIM);
    // Bottom horizontal line
    tft.drawFastHLine(leftX, bottomY, rightX - leftX - radius + 1, RADAR_DIM);

    tft.setTextColor(RADAR_DIM, TFT_BLACK);
    tft.setTextSize(1);

    // Title
    tft.setCursor(panelX, cursorY);
    tft.print("STATUS");

    // Signal strength
    tft.setCursor(panelX, cursorY += (textHeight + 10));
    tft.print("SIG:");
    char sigStr[8];
    snprintf(sigStr, sizeof(sigStr), "%.1f%%", radarState.signalStrength);
    tft.setCursor(panelX, cursorY += textHeight);
    tft.print(sigStr);

    // Bearing
    tft.setCursor(panelX, cursorY += textHeight + 10);
    tft.print("BEARING");
    tft.setCursor(panelX, cursorY += textHeight);
    char bearStr[8];
    snprintf(bearStr, sizeof(bearStr), "%03d", radarState.bearing);
    tft.print(bearStr);
    tft.print((char)247);  // Degree symbol

    // Range indicator
    tft.setCursor(panelX, cursorY += textHeight + 10);
    tft.print("RANGE:");
    tft.setCursor(panelX, cursorY += textHeight);
    tft.print("500M");
}

// Update target states
static void updateTargets() {
    int activeCount = 0;

    for (int i = 0; i < MAX_TARGETS; i++) {
        RadarTarget& target = radarState.targets[i];

        if (target.active) {
            activeCount++;

            // Move target
            target.angle += target.speed * target.direction;
            target.distance += target.distSpeed;

            // Check if target is out of bounds
            if (target.angle < 0 || target.angle > 180 ||
                target.distance < 0 || target.distance > 1.0f) {
                target.active = false;
                activeCount--;
            }

            // Check for sweep intersection (within 5 degrees)
            float angleDiff = abs(target.angle - radarState.sweepAngle);
            if (angleDiff < 5.0f) {
                target.pulsing = true;
                target.pulseCount++;
            } else {
                target.pulsing = false;
                target.pulseCount = 0;
            }
        }
    }

    radarState.contactCount = activeCount;

    // Randomly spawn new targets
    if (random(TARGET_SPAWN_CHANCE) == 0) {
        for (int i = 0; i < MAX_TARGETS; i++) {
            if (!radarState.targets[i].active) {
                RadarTarget& target = radarState.targets[i];
                target.active = true;
                target.direction = (random(2) == 0) ? 1 : -1;
                target.angle = (target.direction == 1) ? 5.0f : 175.0f;
                target.distance = 0.3f + (random(50) / 100.0f);  // 0.3 to 0.8
                target.speed = 0.2f + (random(25) / 100.0f);     // 0.2 to 0.45
                target.distSpeed = (random(20) - 10) / 500.0f;   // Small radial drift
                target.pulsing = false;
                target.pulseCount = 0;
                break;  // Only spawn one at a time
            }
        }
    }
}

void initRadarScreenSaver() {
    radarState.sweepAngle = 90.0f;
    radarState.sweepDirection = 1;
    radarState.lastUpdate = 0;
    radarState.lastDataChange = 0;
    radarState.initialized = false;
    radarState.contactCount = 0;
    radarState.scanCycle = 0;
    radarState.signalStrength = 98.5f;
    radarState.bearing = 0;

    // Initialize all targets as inactive
    for (int i = 0; i < MAX_TARGETS; i++) {
        radarState.targets[i].active = false;
        radarState.targets[i].pulsing = false;
        radarState.targets[i].pulseCount = 0;
    }

    // Clean up existing sprite if any
    if (radarSprite != nullptr) {
        radarSprite->deleteSprite();
        delete radarSprite;
        radarSprite = nullptr;
    }
}

void renderRadarScreenSaver(TFT_eSPI& tft) {
    unsigned long now = millis();

    // First-time initialization
    if (!radarState.initialized) {
        initRadarScreenSaver();
        tft.fillScreen(TFT_BLACK);

        // Create sprite for radar area only (side panels draw directly to TFT)
        // Using 8-bit color depth to reduce memory (~35KB instead of ~70KB)
        radarSprite = new TFT_eSprite(&tft);
        radarSprite->setColorDepth(8);
        if (!radarSprite->createSprite(RADAR_SPRITE_WIDTH, RADAR_SPRITE_HEIGHT)) {
            // Sprite creation failed - fall back to no sprite
            delete radarSprite;
            radarSprite = nullptr;
        }

        radarState.initialized = true;
        radarState.lastUpdate = now;
        radarState.lastDataChange = now;
    }

    // Update at ~30fps (every 33ms)
    if (now - radarState.lastUpdate < 33) {
        return;
    }
    radarState.lastUpdate = now;

    // Update sweep angle
    radarState.sweepAngle += SWEEP_SPEED * radarState.sweepDirection;
    if (radarState.sweepAngle >= 179.0f) {
        radarState.sweepAngle = 179.0f;
        radarState.sweepDirection = -1;
        radarState.scanCycle++;
    } else if (radarState.sweepAngle <= 1.0f) {
        radarState.sweepAngle = 1.0f;
        radarState.sweepDirection = 1;
        radarState.scanCycle++;
    }

    // Update bearing to follow sweep
    radarState.bearing = (int)radarState.sweepAngle;

    // Update targets
    updateTargets();

    // Occasionally vary signal strength for realism
    if (now - radarState.lastDataChange > 2000) {
        radarState.signalStrength = 95.0f + (random(50) / 10.0f);  // 95.0 to 100.0
        radarState.lastDataChange = now;
    }

    // Only use sprite if it was successfully created
    if (radarSprite != nullptr) {
        // Clear sprite and draw radar elements to it
        radarSprite->fillSprite(TFT_BLACK);
        drawRadarArcs(*radarSprite);
        drawSweepLine(*radarSprite);
        drawTargets(*radarSprite);
        drawBorders(*radarSprite);

        // Push sprite to display at radar area position
        radarSprite->pushSprite(RADAR_LEFT_BORDER, 0);
    }

    // Draw side panels directly to TFT (no sprite needed - static content)
    tft.startWrite();
    drawLeftPanel(tft);
    drawRightPanel(tft);
    tft.endWrite();
}
