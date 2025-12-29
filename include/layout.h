#pragma once

// Display dimensions (ST7789 170x320 in landscape)
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 170

// Menu button layout
#define MENU_BTN_HEIGHT 35
#define MENU_BTN_RADIUS 6
#define MENU_BTN_GAP 7
#define MENU_START_Y 5
#define MENU_FONT_SIZE 2
#define MENU_VIEWPORT_SIZE 4

// Sidebar layout (positioned from right edge)
#define SIDEBAR_WIDTH 85
#define SIDEBAR_X (SCREEN_WIDTH - SIDEBAR_WIDTH)

// Menu content area (left of sidebar)
#define MENU_CONTENT_WIDTH (SIDEBAR_X - 15)
