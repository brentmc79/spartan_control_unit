#include "menu_system.h"
#include "theme.h"

// --- Menu Definitions ---
// Initializer order: {label, type, subMenu, subMenuSize, options, numOptions, action, currentOption}

// --- VISOR SUBMENU ---
const char* visorOnOffOptions[] = {"Off", "On"};
const char* visorModeOptions[] = {"Solid", "Flashing", "Pulsing"};
const char* visorColorOptions[] = {"White", "Blue", "Green", "Yellow", "Orange", "Red"};
const char* visorBrightnessOptions[] = {"1", "2", "3", "4"};
MenuItem visorMenuItems[] = {
    {"On/Off", MenuItemType::TOGGLE, nullptr, 0, visorOnOffOptions, 2, nullptr, 0},
    {"Mode", MenuItemType::CYCLE, nullptr, 0, visorModeOptions, 3, nullptr, 0},
    {"Color", MenuItemType::CYCLE, nullptr, 0, visorColorOptions, 6, nullptr, 0},
    {"Brightness", MenuItemType::CYCLE, nullptr, 0, visorBrightnessOptions, 4, nullptr, 0},
    {"<- Back", MenuItemType::BACK, nullptr, 0, nullptr, 0, nullptr, 0}
};

// --- THERMALS SUBMENU ---
const char* thermalsOnOffOptions[] = {"Off", "On"};
MenuItem thermalsMenuItems[] = {
    {"On/Off", MenuItemType::TOGGLE, nullptr, 0, thermalsOnOffOptions, 2, nullptr, 0},
    {"<- Back", MenuItemType::BACK, nullptr, 0, nullptr, 0, nullptr, 0}
};

// --- HUD SUBMENU ---
const char* hudOptions[] = {"Biometric", "Radar", "Matrix"};
MenuItem hudMenuItems[] = {
    {"Display", MenuItemType::CYCLE, nullptr, 0, hudOptions, 3, nullptr, 0},
    {"<- Back", MenuItemType::BACK, nullptr, 0, nullptr, 0, nullptr, 0}
};

// --- SETTINGS SUBMENU ---
const char* bootSeqOptions[] = {"UNSC Logo", "Progress Bar"};
MenuItem settingsMenuItems[] = {
    {"Boot Sequence", MenuItemType::CYCLE, nullptr, 0, bootSeqOptions, 2, nullptr, 0},
    {"<- Back", MenuItemType::BACK, nullptr, 0, nullptr, 0, nullptr, 0}
};

// --- MAIN MENU ---
MenuItem mainMenuItems[] = {
    {"VISOR",    MenuItemType::SUBMENU, visorMenuItems,    sizeof(visorMenuItems) / sizeof(MenuItem),    nullptr, 0, nullptr, 0},
    {"THERMALS", MenuItemType::SUBMENU, thermalsMenuItems, sizeof(thermalsMenuItems) / sizeof(MenuItem), nullptr, 0, nullptr, 0},
    {"HUD",      MenuItemType::SUBMENU, hudMenuItems,      sizeof(hudMenuItems) / sizeof(MenuItem),      nullptr, 0, nullptr, 0},
    {"SETTINGS", MenuItemType::SUBMENU, settingsMenuItems, sizeof(settingsMenuItems) / sizeof(MenuItem), nullptr, 0, nullptr, 0}
};
const int mainMenuItemCount = sizeof(mainMenuItems) / sizeof(MenuItem);


// --- Controller Implementation ---

MenuController::MenuController(MenuItem* rootMenu, int rootMenuSize, TFT_eSPI& tft) : tft(tft) {
    navigationStack.push_back({rootMenu, rootMenuSize, 0});
}

void MenuController::navigateTo(MenuItem* menu, int size) {
    if (menu && size > 0) {
        navigationStack.push_back({menu, size, 0});
        isDirty = true;
    }
}

void MenuController::back() {
    if (navigationStack.size() > 1) {
        navigationStack.pop_back();
        isDirty = true;
    }
}

void MenuController::nextItem() {
    MenuState& currentState = navigationStack.back();
    currentState.selectedIndex = (currentState.selectedIndex + 1) % currentState.menuSize;
    isDirty = true;
}

void MenuController::selectItem() {
    MenuState& currentState = navigationStack.back();
    MenuItem& selected = currentState.menu[currentState.selectedIndex];

    switch (selected.type) {
        case MenuItemType::SUBMENU:
            navigateTo(selected.subMenu, selected.subMenuSize);
            break;
        case MenuItemType::TOGGLE:
        case MenuItemType::CYCLE:
            selected.currentOption = (selected.currentOption + 1) % selected.numOptions;
            // Here you would trigger the ESP-NOW update
            break;
        case MenuItemType::ACTION:
            if (selected.action) {
                selected.action(this);
            }
            break;
        case MenuItemType::BACK:
            back();
            break;
    }
    isDirty = true;
}

void MenuController::render() {
    if (!isDirty) return;

    tft.startWrite();

    tft.fillScreen(HEX_BG);
    renderMenuItems();
    renderSidebar();

    tft.endWrite();

    isDirty = false;
}

void MenuController::renderSidebar() {
    tft.drawLine(235, 0, 235, 165, HEX_MUTED);
    tft.drawLine(236, 0, 236, 165, HEX_BORDER);
    tft.drawLine(237, 0, 237, 165, HEX_MUTED);
    tft.drawLine(238, 0, 238, 165, HEX_MUTED);
    tft.drawLine(239, 5, 239, 170, HEX_BORDER);
    tft.drawLine(240, 5, 240, 170, HEX_MUTED);
}

#define BTN_HEIGHT 35
#define BTN_RADIUS 6
#define FONT_SIZE 2

void MenuController::renderMenuItems() {
    MenuState& currentState = navigationStack.back();
    int startX = 0;
    int startY = 5;
    int gap = 7;
    int width = 220;

    for (int i = 0; i < currentState.menuSize; i++) {
        MenuItem& item = currentState.menu[i];
        bool isActive = (i == currentState.selectedIndex);
        int currentY = startY + (i * (BTN_HEIGHT + gap));

        uint16_t fillColor = isActive ? HEX_BORDER : HEX_MUTED;
        uint16_t textColor = isActive ? HEX_BG : HEX_TEXT_PRI;

        tft.fillRoundRect(startX + 2, currentY + 2, width - 4, BTN_HEIGHT - 4, BTN_RADIUS - 2, fillColor);
        tft.drawRoundRect(startX, currentY, width, BTN_HEIGHT, BTN_RADIUS, HEX_BORDER);
        
        if (isActive) {
             tft.drawLine(startX+width-40, currentY+BTN_HEIGHT-3, startX+width-4, currentY+BTN_HEIGHT-3, TFT_BLACK);
             tft.drawLine(startX+width-39, currentY+BTN_HEIGHT-4, startX+width-4, currentY+BTN_HEIGHT-4, TFT_BLACK);
        }

        String label = String(item.label);
        if (item.type == MenuItemType::TOGGLE || item.type == MenuItemType::CYCLE) {
            label += ": " + String(item.options[item.currentOption]);
        }
        String finalLabel = isActive ? "> " + label + " <" : label;

        tft.setTextColor(textColor);
        tft.setTextSize(FONT_SIZE);
        tft.setTextDatum(ML_DATUM);
        tft.drawString(finalLabel, startX + 15, currentY + BTN_HEIGHT / 2);
    }
}
