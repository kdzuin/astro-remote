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
        bool enabled = true; // If false, item is shown but can't be selected
    };

    SelectableList() : title("Menu") {}
    explicit SelectableList(const std::string &customTitle) : title(customTitle) {}

    void clear();
    void addItem(const std::string &label, bool enabled = true);
    void setTitle(const std::string &newTitle) { title = newTitle; }
    void setSelectedIndex(int index);

    int getSelectedIndex() const { return selectedIndex; }
    const Item &getSelectedItem() const { return items[selectedIndex]; }
    const char *getItem(int index) const { return items[index].label.c_str(); }
    bool isEmpty() const { return items.empty(); }
    size_t size() const { return items.size(); }

    // Returns true if selection changed
    bool selectNext()
    {
        if (selectedIndex < items.size() - 1)
        {
            selectedIndex++;
            return true;
        }

        selectedIndex = 0;
        return true;
    }

    // Draw the list
    void draw();

private:
    std::vector<Item> items;
    std::string title;
    int selectedIndex = 0;
};
