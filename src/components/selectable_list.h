#pragma once

#include <string>
#include <vector>

#include "utils/colors.h"

// Forward declaration
namespace MenuSystem {}

template <typename IdType>
class SelectableList {
private:
    // Display constants
    const int ITEM_HEIGHT = 14;        // Height per item
    const int HORIZONTAL_PADDING = 8;  // Space for selection marker
    const int ITEM_PADDING = 2;        // Padding between items

    // Helper function to create color
    static uint32_t defaultColor() { return colors::get(colors::WHITE); }

public:
    struct Info {
        std::string text;
        uint32_t* color = nullptr;  // nullptr means use current text color

        Info(const std::string& infoText) : text(infoText) {}
        Info(const std::string& infoText, uint32_t infoColor)
            : text(infoText), color(new uint32_t(infoColor)) {}

        ~Info() { delete color; }

        // Copy constructor
        Info(const Info& other) : text(other.text) {
            if (other.color) {
                color = new uint32_t(*other.color);
            }
        }

        // Assignment operator
        Info& operator=(const Info& other) {
            if (this != &other) {
                text = other.text;
                delete color;
                color = other.color ? new uint32_t(*other.color) : nullptr;
            }
            return *this;
        }
    };

    struct Item {
        IdType id;               // Unique identifier for the item (enum)
        std::string label;       // Display text
        bool enabled = true;     // If false, item is shown but can't be selected
        bool separator = false;  // If true, item is a separator line
        Info* info = nullptr;    // Optional info section (nullptr if not present)

        Item(IdType itemId, const std::string& itemLabel, bool itemEnabled = true)
            : id(itemId), label(itemLabel), enabled(itemEnabled) {}

        Item(IdType itemId, const std::string& itemLabel, const std::string& infoText,
             uint32_t infoColor, bool itemEnabled = true)
            : id(itemId), label(itemLabel), enabled(itemEnabled) {
            info = new Info(infoText, infoColor);
        }

        Item(IdType itemId, const std::string& itemLabel, const std::string& infoText,
             bool itemEnabled = true)
            : id(itemId), label(itemLabel), enabled(itemEnabled) {
            info = new Info(infoText);
        }

        // Constructor for separator
        Item() : enabled(false), separator(true) {}

        // Copy constructor
        Item(const Item& other)
            : id(other.id), label(other.label), enabled(other.enabled), separator(other.separator) {
            if (other.info) {
                info = new Info(*other.info);
            }
        }

        // Assignment operator
        Item& operator=(const Item& other) {
            if (this != &other) {
                id = other.id;
                label = other.label;
                enabled = other.enabled;
                separator = other.separator;
                delete info;
                if (other.info) {
                    info = new Info(*other.info);
                } else {
                    info = nullptr;
                }
            }
            return *this;
        }

        // Destructor
        ~Item() { delete info; }
    };

    SelectableList();
    explicit SelectableList(const std::string& customTitle);

    void clear();
    void addItem(IdType id, const std::string& label, bool enabled = true);
    void addItem(IdType id, const std::string& label, const std::string& infoText,
                 uint32_t infoColor, bool enabled = true);
    void addItem(IdType id, const std::string& label, const std::string& infoText,
                 bool enabled = true);
    void addSeparator();
    void setTitle(const std::string& newTitle);
    void setSelectedIndex(int index);
    IdType getSelectedId() const;
    const Item& getSelectedItem() const;
    const char* getItem(int index) const;
    bool isEmpty() const;
    size_t size() const;
    bool selectNext();
    bool selectPrev();
    void draw();
    int getSelectedIndex() const;

private:
    std::vector<Item> items;
    std::string title;
    int selectedIndex = 0;
};

#include "components/selectable_list.tpp"
