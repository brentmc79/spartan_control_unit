#pragma once

#include <TFT_eSPI.h>
#include <vector>
#include <functional>

// Forward-declarations
class MenuController;
struct MenuItem;

// Define callback types
using ActionCallback = std::function<void(MenuController*)>;
using UpdateCallback = std::function<void(MenuItem*)>;

// Defines the different behaviors a menu item can have.
enum class MenuItemType {
    SUBMENU,
    TOGGLE,
    CYCLE,
    ACTION,
    BACK
};

// Represents a single item within a menu.
struct MenuItem {
    const char* label;
    MenuItemType type;

    // Data members for all types (replaces union)
    MenuItem* subMenu;
    int subMenuSize;
    const char** options;
    int numOptions;
    ActionCallback action;
    UpdateCallback onUpdate;

    // State for TOGGLE and CYCLE items
    int currentOption;
};

// Manages the state, navigation, and rendering of the entire menu system.
class MenuController {
public:
    MenuController(MenuItem* rootMenu, int rootMenuSize, TFT_eSPI& tft);

    void render();
    void nextItem();
    void selectItem();

private:
    void renderSidebar();
    void renderMenuItems();
    void navigateTo(MenuItem* menu, int size);
    void back();

    TFT_eSPI& tft;

    struct MenuState {
        MenuItem* menu;
        int menuSize;
        int selectedIndex;
        int scrollOffset = 0;
    };

    std::vector<MenuState> navigationStack;
    bool isDirty = true;
};

// Make menu definitions accessible globally
extern MenuItem mainMenuItems[];
extern const int mainMenuItemCount;