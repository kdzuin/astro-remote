#pragma once

#include <M5Unified.h>
#include <vector>
#include <string>

template <typename IdType>
class SelectableList
{
public:
    struct Item
    {
        IdType id;           // Unique identifier for the item (enum)
        std::string label;   // Display text
        bool enabled = true; // If false, item is shown but can't be selected

        Item(IdType itemId, const std::string &itemLabel, bool itemEnabled = true)
            : id(itemId), label(itemLabel), enabled(itemEnabled) {}
    };

    SelectableList();
    explicit SelectableList(const std::string &customTitle);

    void clear();
    void addItem(IdType id, const std::string &label, bool enabled = true);
    void setTitle(const std::string &newTitle);
    void setSelectedIndex(int index);
    IdType getSelectedId() const;
    const Item &getSelectedItem() const;
    const char *getItem(int index) const;
    bool isEmpty() const;
    size_t size() const;
    bool selectNext();
    void draw();
    int getSelectedIndex() const;

private:
    std::vector<Item> items;
    std::string title;
    int selectedIndex = 0;
};

#include "selectable_list.tpp"
