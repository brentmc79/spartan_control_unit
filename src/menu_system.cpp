#include "menu_system.h"
#include "theme.h"
#include "state.h"
#include "communication.h"

extern void saveAppState(); // Forward declaration for saving app state"

// --- Callback Functions ---

void onVisorToggle(MenuItem* item) {
    appState.visorOn = item->currentOption;
    sendStateUpdate();
    saveAppState();
}

void onVisorModeChange(MenuItem* item) {
    appState.visorMode = (VisorMode)item->currentOption;
    sendStateUpdate();
    saveAppState();
}

void onVisorColorChange(MenuItem* item) {
    appState.visorColor = (VisorColor)item->currentOption;
    sendStateUpdate();
    saveAppState();
}

void onVisorBrightnessChange(MenuItem* item) {
    appState.visorBrightness = item->currentOption + 1; // Options are "1", "2", etc.
    sendStateUpdate();
    saveAppState();
}

void onThermalsToggle(MenuItem* item) {
    appState.thermalsOn = item->currentOption;
    sendStateUpdate();
    saveAppState();
}

void onHudChange(MenuItem* item) {
    appState.hudStyle = (HudStyle)item->currentOption;
    // This is a local-only change, no need to send ESP-NOW update
    saveAppState();
}

void onBootSeqChange(MenuItem* item) {
    appState.bootSequence = (BootSequence)item->currentOption;
    // Also a local-only change
    saveAppState();
}


// --- Menu Definitions ---
// Initializer order: {label, type, subMenu, subMenuSize, options, numOptions, action, onUpdate, currentOption}

// --- VISOR SUBMENU ---
const char* visorOnOffOptions[] = {"Off", "On"};
const char* visorModeOptions[] = {"Solid", "Flashing", "Pulsing", "Strobe"};
const char* visorColorOptions[] = {"White", "Blue", "Green", "Yellow", "Orange", "Red"};
const char* visorBrightnessOptions[] = {"1", "2", "3", "4"};
MenuItem visorMenuItems[] = {
    {"On/Off",     MenuItemType::TOGGLE, nullptr, 0, visorOnOffOptions,      2, nullptr, onVisorToggle,          0},
    {"Mode",       MenuItemType::CYCLE,  nullptr, 0, visorModeOptions,       4, nullptr, onVisorModeChange,      0},
    {"Color",      MenuItemType::CYCLE,  nullptr, 0, visorColorOptions,      6, nullptr, onVisorColorChange,     0},
    {"Brightness", MenuItemType::CYCLE,  nullptr, 0, visorBrightnessOptions, 4, nullptr, onVisorBrightnessChange,0},
    {"<- Back",    MenuItemType::BACK,   nullptr, 0, nullptr,                0, nullptr, nullptr,                0}
};

// --- THERMALS SUBMENU ---
const char* thermalsOnOffOptions[] = {"Off", "On"};
MenuItem thermalsMenuItems[] = {
    {"On/Off",   MenuItemType::TOGGLE, nullptr, 0, thermalsOnOffOptions, 2, nullptr, onThermalsToggle, 0},
    {"<- Back",  MenuItemType::BACK,   nullptr, 0, nullptr,              0, nullptr, nullptr,          0}
};

// --- HUD SUBMENU ---
const char* hudOptions[] = {"Biometric", "Radar", "Matrix"};
MenuItem hudMenuItems[] = {
    {"Display",  MenuItemType::CYCLE,  nullptr, 0, hudOptions, 3, nullptr, onHudChange, 0},
    {"<- Back",  MenuItemType::BACK,   nullptr, 0, nullptr,    0, nullptr, nullptr,     0}
};

// --- SETTINGS SUBMENU ---
const char* bootSeqOptions[] = {"UNSC Logo", "Progress Bar"};
MenuItem settingsMenuItems[] = {
    {"Boot Sequence", MenuItemType::CYCLE,  nullptr, 0, bootSeqOptions, 2, nullptr, onBootSeqChange, 0},
    {"<- Back",       MenuItemType::BACK,   nullptr, 0, nullptr,        0, nullptr, nullptr,         0}
};

// --- MAIN MENU ---
MenuItem mainMenuItems[] = {
    {"VISOR",    MenuItemType::SUBMENU, visorMenuItems,    sizeof(visorMenuItems) / sizeof(MenuItem),    nullptr, 0, nullptr, nullptr, 0},
    {"THERMALS", MenuItemType::SUBMENU, thermalsMenuItems, sizeof(thermalsMenuItems) / sizeof(MenuItem), nullptr, 0, nullptr, nullptr, 0},
    {"HUD",      MenuItemType::SUBMENU, hudMenuItems,      sizeof(hudMenuItems) / sizeof(MenuItem),      nullptr, 0, nullptr, nullptr, 0},
    {"SETTINGS", MenuItemType::SUBMENU, settingsMenuItems, sizeof(settingsMenuItems) / sizeof(MenuItem), nullptr, 0, nullptr, nullptr, 0}
};
const int mainMenuItemCount = sizeof(mainMenuItems) / sizeof(MenuItem);

void updateMenuFromState() {
    // Visor Settings
    visorMenuItems[0].currentOption = appState.visorOn ? 1 : 0;
    visorMenuItems[1].currentOption = (int)appState.visorMode;
    visorMenuItems[2].currentOption = (int)appState.visorColor;
    visorMenuItems[3].currentOption = appState.visorBrightness - 1;

    // Thermals
    thermalsMenuItems[0].currentOption = appState.thermalsOn ? 1 : 0;

    // HUD
    hudMenuItems[0].currentOption = (int)appState.hudStyle;

    // Settings
    settingsMenuItems[0].currentOption = (int)appState.bootSequence;
}


// --- Controller Implementation ---

MenuController::MenuController(MenuItem* rootMenu, int rootMenuSize, TFT_eSPI& tft) : tft(tft) {
    navigationStack.push_back({rootMenu, rootMenuSize, 0});
}

void MenuController::navigateTo(MenuItem* menu, int size) {
    if (menu && size > 0) {
        navigationStack.push_back({menu, size, 0, 0});
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

    const int VIEWPORT_SIZE = 4;
    if (currentState.selectedIndex == 0) {
        currentState.scrollOffset = 0;
    } else if (currentState.selectedIndex >= currentState.scrollOffset + VIEWPORT_SIZE) {
        currentState.scrollOffset = currentState.selectedIndex - VIEWPORT_SIZE + 1;
    }

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
            if (selected.onUpdate) {
                selected.onUpdate(&selected);
            }
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
    const int VIEWPORT_SIZE = 4;
    int startX = 0;
    int startY = 5;
    int gap = 7;
    int width = 220;

    int viewportEnd = currentState.scrollOffset + VIEWPORT_SIZE;
    if (viewportEnd > currentState.menuSize) {
        viewportEnd = currentState.menuSize;
    }

    for (int i = currentState.scrollOffset; i < viewportEnd; i++) {
        MenuItem& item = currentState.menu[i];
        bool isActive = (i == currentState.selectedIndex);
        
        int viewportIndex = i - currentState.scrollOffset;
        int currentY = startY + (viewportIndex * (BTN_HEIGHT + gap));

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
