#pragma once

#include "utils/display_constants.h"

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
                                     const std::string& infoText, unifiedColor infoColor,
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
    auto& display = MenuSystem::getHardware()->getDisplay();
    const unifiedColor SELECTED_BG = display.getColor(display::colors::WHITE);
    const unifiedColor SELECTED_FG = display.getColor(display::colors::BLACK);
    const unifiedColor NORMAL_BG = display.getColor(display::colors::BLACK);
    const unifiedColor NORMAL_FG = display.getColor(display::colors::WHITE);
    const unifiedColor DISABLED_FG = display.getColor(display::colors::GRAY_500);

    int y = 0;

    display.fillScreen(display.getColor(display::colors::BLACK));

    // Draw title if present
    if (!title.empty()) {
        display.setTextSize(1.25);
        display.setTextAlignment(textAlign::middle_center);
        display.setTextColor(NORMAL_FG);
        display.drawString(title.c_str(), display.width() / 2, y + ITEM_HEIGHT / 1.25);
        y += ITEM_HEIGHT;
    }

    // Draw all items
    for (size_t i = 0; i < items.size(); i++) {
        const auto& item = items[i];
        const bool isSelected = (i == selectedIndex);

        if (item.separator) {
            // Draw separator line in the middle of the item height
            int lineY = y + ITEM_HEIGHT / 2;
            display.drawLine(0, lineY, display.width(), lineY, DISABLED_FG);
        } else {
            const unifiedColor bgColor = isSelected ? SELECTED_BG : NORMAL_BG;
            const unifiedColor fgColor =
                item.enabled ? (isSelected ? SELECTED_FG : NORMAL_FG) : DISABLED_FG;

            // Draw selection background
            display.fillRect(0, y, display.width(), ITEM_HEIGHT, bgColor);

            display.setTextAlignment(textAlign::middle_left);
            display.setTextColor(fgColor);
            display.drawString(item.label.c_str(), HORIZONTAL_PADDING, y + ITEM_HEIGHT / 2);

            // Draw info text if present
            if (item.info) {
                display.setTextColor(item.info->color ? *item.info->color : fgColor);
                display.setTextAlignment(textAlign::middle_right);
                display.drawString(item.info->text.c_str(), display.width() - HORIZONTAL_PADDING,
                                   y + ITEM_HEIGHT / 2);
            }
        }

        y += ITEM_HEIGHT + ITEM_PADDING;
    }
}
