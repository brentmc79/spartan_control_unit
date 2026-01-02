#pragma once

// Display dimensions (ST7789 170x320 in landscape)
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 170

// LED brightness settings
// Brightness levels 1-4 map to values 63-252 (step size = 255/4)
#define BRIGHTNESS_LEVELS 4
#define MAX_BRIGHTNESS 255
#define BRIGHTNESS_STEP (MAX_BRIGHTNESS / BRIGHTNESS_LEVELS)

// LED effect timing (milliseconds)
#define FLASH_INTERVAL_MS 500
#define STROBE_INTERVAL_MS 100

// Communication timing (milliseconds)
// Heartbeat keeps receiver's watchdog happy; timeout triggers safety shutdown
#define HEARTBEAT_INTERVAL_MS 5000
#define RECEIVER_TIMEOUT_MS 10000

// Screen saver timing (milliseconds)
#define SCREENSAVER_TIMEOUT_MS 5000

// Safety shutdown animation timing (milliseconds)
#define SHUTDOWN_FLASH_DURATION_MS 5000
#define SHUTDOWN_FLASH_INTERVAL_MS 200
#define SHUTDOWN_FADE_DURATION_MS 5000

// Menu button layout
#define MENU_BTN_HEIGHT 25
#define MENU_BTN_RADIUS 6
#define MENU_BTN_GAP 7
#define MENU_START_Y 5
#define MENU_FONT_SIZE 2
#define MENU_VIEWPORT_SIZE 5

// Sidebar layout (positioned from right edge)
#define SIDEBAR_WIDTH 85
#define SIDEBAR_X (SCREEN_WIDTH - SIDEBAR_WIDTH)

// Menu content area (left of sidebar)
#define MENU_CONTENT_WIDTH (SIDEBAR_X - 15)
