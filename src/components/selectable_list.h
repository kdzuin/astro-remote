#pragma once

#include <M5Unified.h>
#include <vector>
#include <string>

template <typename IdType>
class SelectableList
{
private:
    // Display constants
    const int ITEM_HEIGHT = 14;       // Height per item
    const int HORIZONTAL_PADDING = 8; // Space for selection marker
    const int ITEM_PADDING = 2;       // Padding between items
    const int TITLE_PADDING = 4;      // Extra padding below title
    const uint16_t SELECTED_BG = WHITE;
    const uint16_t SELECTED_FG = BLACK;
    const uint16_t NORMAL_BG = BLACK;
    const uint16_t NORMAL_FG = WHITE;
    const uint16_t DISABLED_FG = M5.Display.color565(128, 128, 128); // Gray

public:
    struct Info
    {
        std::string text;
        uint16_t* color = nullptr;  // nullptr means use current text color

        Info(const std::string& infoText) : text(infoText) {}
        Info(const std::string& infoText, uint16_t infoColor) 
            : text(infoText), color(new uint16_t(infoColor)) {}
        
        ~Info() { delete color; }
        
        // Copy constructor
        Info(const Info& other) : text(other.text) {
            if (other.color) {
                color = new uint16_t(*other.color);
            }
        }
        
        // Assignment operator
        Info& operator=(const Info& other) {
            if (this != &other) {
                text = other.text;
                delete color;
                if (other.color) {
                    color = new uint16_t(*other.color);
                } else {
                    color = nullptr;
                }
            }
            return *this;
        }
    };

    struct Item
    {
        IdType id;            // Unique identifier for the item (enum)
        std::string label;    // Display text
        bool enabled = true;  // If false, item is shown but can't be selected
        Info *info = nullptr; // Optional info section (nullptr if not present)

        Item(IdType itemId, const std::string &itemLabel, bool itemEnabled = true)
            : id(itemId), label(itemLabel), enabled(itemEnabled) {}

        Item(IdType itemId, const std::string &itemLabel, const std::string &infoText, uint16_t infoColor = TFT_WHITE, bool itemEnabled = true)
            : id(itemId), label(itemLabel), enabled(itemEnabled)
        {
            info = new Info(infoText, infoColor);
        }

        Item(IdType itemId, const std::string &itemLabel, const std::string &infoText, bool itemEnabled = true)
            : id(itemId), label(itemLabel), enabled(itemEnabled)
        {
            info = new Info(infoText);
        }

        // Copy constructor
        Item(const Item &other)
            : id(other.id), label(other.label), enabled(other.enabled)
        {
            if (other.info)
            {
                info = new Info(*other.info);
            }
        }

        // Assignment operator
        Item &operator=(const Item &other)
        {
            if (this != &other)
            {
                id = other.id;
                label = other.label;
                enabled = other.enabled;
                delete info;
                if (other.info)
                {
                    info = new Info(*other.info);
                }
                else
                {
                    info = nullptr;
                }
            }
            return *this;
        }

        // Destructor
        ~Item()
        {
            delete info;
        }
    };

    SelectableList();
    explicit SelectableList(const std::string &customTitle);

    void clear();
    void addItem(IdType id, const std::string &label, bool enabled = true);
    void addItem(IdType id, const std::string &label, const std::string &infoText, uint16_t infoColor = TFT_WHITE, bool enabled = true);
    void addItem(IdType id, const std::string &label, const std::string &infoText, bool enabled = true);
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
