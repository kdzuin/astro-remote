#pragma once

#include "utils/colors.h"

template <typename IdType>
SelectableList<IdType>::SelectableList() : title("Menu") {}

template <typename IdType>
SelectableList<IdType>::SelectableList(const std::string& customTitle) : title(customTitle) {}

template <typename IdType>
void SelectableList<IdType>::clear() {
    items.clear();
    selectedIndex = 0;
    // Add default separator at the beginning
    if (!title.empty()) {
        addSeparator();
    }
}

template <typename IdType>
void SelectableList<IdType>::addItem(IdType id, const std::string& label, bool enabled) {
    items.emplace_back(id, label, enabled);
}

template <typename IdType>
void SelectableList<IdType>::addItem(IdType id, const std::string& label,
                                     const std::string& infoText, uint32_t infoColor,
                                     bool enabled) {
    items.emplace_back(id, label, infoText, infoColor, enabled);
}

template <typename IdType>
void SelectableList<IdType>::addItem(IdType id, const std::string& label,
                                     const std::string& infoText, bool enabled) {
    items.emplace_back(id, label, infoText, enabled);
}

template <typename IdType>
void SelectableList<IdType>::addSeparator() {
    items.emplace_back();
}

template <typename IdType>
void SelectableList<IdType>::setTitle(const std::string& newTitle) {
    title = newTitle;
}

template <typename IdType>
void SelectableList<IdType>::setSelectedIndex(int index) {
    if (index >= 0 && index < items.size()) {
        selectedIndex = index;
    }
}

template <typename IdType>
IdType SelectableList<IdType>::getSelectedId() const {
    return items.empty() ? IdType{} : items[selectedIndex].id;
}

template <typename IdType>
const typename SelectableList<IdType>::Item& SelectableList<IdType>::getSelectedItem() const {
    return items[selectedIndex];
}

template <typename IdType>
const char* SelectableList<IdType>::getItem(int index) const {
    return items[index].label.c_str();
}

template <typename IdType>
bool SelectableList<IdType>::isEmpty() const {
    return items.empty();
}

template <typename IdType>
size_t SelectableList<IdType>::size() const {
    return items.size();
}

template <typename IdType>
bool SelectableList<IdType>::selectNext() {
    if (items.empty()) {
        return false;
    }

    size_t startIndex = selectedIndex;
    do {
        selectedIndex = (selectedIndex + 1) % items.size();
        if (!items[selectedIndex].separator) {
            return true;
        }
    } while (selectedIndex != startIndex);

    // If we got here, try to find any non-separator item
    for (size_t i = 0; i < items.size(); i++) {
        if (!items[i].separator) {
            selectedIndex = i;
            return true;
        }
    }

    return false;  // No non-separator items found
}

template <typename IdType>
bool SelectableList<IdType>::selectPrev() {
    if (items.empty()) {
        return false;
    }

    size_t startIndex = selectedIndex;
    do {
        selectedIndex = (selectedIndex + items.size() - 1) % items.size();
        if (!items[selectedIndex].separator) {
            return true;
        }
    } while (selectedIndex != startIndex);

    // If we got here, try to find any non-separator item
    for (size_t i = 0; i < items.size(); i++) {
        if (!items[i].separator) {
            selectedIndex = i;
            return true;
        }
    }

    return false;  // No non-separator items found
}

template <typename IdType>
int SelectableList<IdType>::getSelectedIndex() const {
    return selectedIndex;
}

template <typename IdType>
void SelectableList<IdType>::draw() {
    const uint32_t SELECTED_BG = colors::get(colors::WHITE);
    const uint32_t SELECTED_FG = colors::get(colors::BLACK);
    const uint32_t NORMAL_BG = colors::get(colors::BLACK);
    const uint32_t NORMAL_FG = colors::get(colors::WHITE);
    const uint32_t DISABLED_FG = colors::get(colors::GRAY_500);

    int y = 0;

    M5.Display.fillScreen(colors::get(colors::BLACK));

    // Draw title if present
    if (!title.empty()) {
        M5.Display.setTextSize(1.25);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextColor(NORMAL_FG);
        M5.Display.drawString(title.c_str(), M5.Display.width() / 2, y + ITEM_HEIGHT / 1.25);
        y += ITEM_HEIGHT;
    }

    // Draw all items
    for (size_t i = 0; i < items.size(); i++) {
        const auto& item = items[i];
        const bool isSelected = (i == selectedIndex);

        if (item.separator) {
            // Draw separator line in the middle of the item height
            int lineY = y + ITEM_HEIGHT / 2;
            M5.Display.drawLine(0, lineY, M5.Display.width(), lineY, DISABLED_FG);
        } else {
            const uint32_t bgColor = isSelected ? SELECTED_BG : NORMAL_BG;
            const uint32_t fgColor =
                item.enabled ? (isSelected ? SELECTED_FG : NORMAL_FG) : DISABLED_FG;

            // Draw selection background
            M5.Display.fillRect(0, y, M5.Display.width(), ITEM_HEIGHT, bgColor);

            M5.Display.setTextDatum(middle_left);
            M5.Display.setTextColor(fgColor);
            M5.Display.drawString(item.label.c_str(), HORIZONTAL_PADDING, y + ITEM_HEIGHT / 2);

            // Draw info text if present
            if (item.info) {
                M5.Display.setTextColor(item.info->color ? *item.info->color : fgColor);
                M5.Display.setTextDatum(middle_right);
                M5.Display.drawString(item.info->text.c_str(),
                                      M5.Display.width() - HORIZONTAL_PADDING, y + ITEM_HEIGHT / 2);
            }
        }

        y += ITEM_HEIGHT + ITEM_PADDING;
    }
}
