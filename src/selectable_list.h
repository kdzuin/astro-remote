#pragma once

#include <M5Unified.h>
#include <vector>
#include <string>
#include <functional>

class SelectableList
{
public:
    struct Item
    {
        std::string label;
        std::string description; // Optional description shown below the label
        bool enabled = true;     // If false, item is shown but can't be selected
    };

    SelectableList() = default;

    void clear();
    void addItem(const std::string &label, const std::string &description = "", bool enabled = true);
    void setTitle(const std::string &newTitle) { title = newTitle; }
    void setActionLabel(const std::string &label) { actionLabel = label; }
    void setSelectedIndex(int index);

    int getSelectedIndex() const { return selectedIndex; }
    const Item &getSelectedItem() const { return items[selectedIndex]; }
    bool isEmpty() const { return items.empty(); }
    size_t size() const { return items.size(); }

    // Returns true if selection changed
    bool next();

    // Draw the list at current cursor position
    void draw(int startY = 0, bool fullRedraw = true);

private:
    std::vector<Item> items;
    int selectedIndex = 0;
    int prevSelectedIndex = 0; // Track previous selection for partial redraw
    std::string title;
    std::string actionLabel = "Select"; // Default action label

    // Display constants
    static const int ITEM_HEIGHT = 14;        // Height per item
    static const int DESCRIPTION_OFFSET = 12; // Indent for description
    static const int SELECTION_MARGIN = 8;    // Space for selection marker
    static const int ITEM_PADDING = 2;        // Padding between items
    static const uint16_t SELECTED_BG = WHITE;
    static const uint16_t SELECTED_FG = BLACK;
    static const uint16_t NORMAL_BG = BLACK;
    static const uint16_t NORMAL_FG = WHITE;
    static const uint16_t DISABLED_FG = 0x8410; // Gray
};
