#pragma once

#include <TFT_eSPI.h>
#include <vector>
#include <functional>

// Forward-declarations
class MenuController;
struct MenuItem;

// Callback invoked when a menu item is selected (receives controller for navigation)
using ActionCallback = std::function<void(MenuController*)>;
// Callback invoked when a menu item's value changes (receives item for state access)
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
// Handles hierarchical menus with support for submenus, toggles, and cyclic options.
class MenuController {
public:
    MenuController(MenuItem* rootMenu, int rootMenuSize, TFT_eSPI& tft);

    // Redraws the menu UI (only when dirty flag is set)
    void render();
    // Moves selection to next menu item, wrapping at end
    void nextItem();
    // Moves selection to previous menu item, wrapping at beginning
    void prevItem();
    // Activates the selected item (toggle, cycle, navigate, or execute action)
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

// Root menu definition
extern MenuItem mainMenuItems[];
extern const int mainMenuItemCount;

// Synchronizes menu item states with appState (call after loading saved state)
void updateMenuFromState();