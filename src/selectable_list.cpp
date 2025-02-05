#include "selectable_list.h"

void SelectableList::clear()
{
    Serial.println("[SelectableList] Clearing list");
    items.clear();
    selectedIndex = 0; // Reset to default
    prevSelectedIndex = 0;
}

void SelectableList::addItem(const std::string &label, const std::string &description, bool enabled)
{
    Serial.printf("[SelectableList] Adding item: %s (enabled: %d)\n", label.c_str(), enabled);
    Item item;
    item.label = label;
    item.description = description;
    item.enabled = enabled;
    items.push_back(item);
}

bool SelectableList::next()
{
    if (items.empty())
    {
        Serial.println("[SelectableList] next() called but list is empty");
        return false;
    }

    Serial.printf("[SelectableList] next() called, current index: %d, items: %d\n", selectedIndex, items.size());

    // Find next enabled item
    int originalIndex = selectedIndex;
    do
    {
        selectedIndex = (selectedIndex + 1) % items.size();
        Serial.printf("[SelectableList] trying index: %d, enabled: %d\n", selectedIndex, items[selectedIndex].enabled);
    } while (!items[selectedIndex].enabled && selectedIndex != originalIndex);

    bool changed = selectedIndex != originalIndex;
    Serial.printf("[SelectableList] next() result: index %d -> %d, changed: %d\n",
                  originalIndex, selectedIndex, changed);
    return changed;
}

void SelectableList::setSelectedIndex(int index)
{
    if (index != selectedIndex)
    {
        prevSelectedIndex = selectedIndex;
        selectedIndex = index;
    }
}

void SelectableList::draw(int startY, bool fullRedraw)
{
    Serial.printf("[SelectableList] Drawing list with %d items, selected: %d, full: %d\n",
                  items.size(), selectedIndex, fullRedraw);
    int y = startY;

    // Draw title if present and doing full redraw
    if (!title.empty() && fullRedraw)
    {
        M5.Display.setTextColor(NORMAL_FG, NORMAL_BG);
        M5.Display.setCursor(SELECTION_MARGIN, y + ITEM_PADDING);
        M5.Display.println(title.c_str());
        y += ITEM_HEIGHT + ITEM_PADDING * 2 + 4;
        M5.Display.drawLine(0, y - 5, M5.Display.width(), y - 5, NORMAL_FG);
    }
    else if (!title.empty())
    {
        y += ITEM_HEIGHT + ITEM_PADDING * 2 + 4;
    }

    // Draw items
    for (size_t i = 0; i < items.size(); i++)
    {
        const auto &item = items[i];
        bool isSelected = (i == selectedIndex);
        bool needsRedraw = fullRedraw || i == selectedIndex || i == prevSelectedIndex;

        if (!needsRedraw)
        {
            // Skip drawing this item if it hasn't changed
            y += ITEM_HEIGHT + (item.description.empty() ? 0 : ITEM_HEIGHT - 4) + ITEM_PADDING * 2;
            continue;
        }

        // Calculate total item height including description
        int itemHeight = ITEM_HEIGHT + (item.description.empty() ? 0 : ITEM_HEIGHT) + ITEM_PADDING * 2;

        // Clear item background first
        M5.Display.fillRect(0, y,
                            M5.Display.width(),
                            itemHeight,
                            NORMAL_BG);

        // Set colors and draw selection background if selected
        if (!item.enabled)
        {
            M5.Display.setTextColor(DISABLED_FG, NORMAL_BG);
        }
        else if (isSelected)
        {
            M5.Display.setTextColor(SELECTED_FG, SELECTED_BG);
            M5.Display.fillRect(0, y,
                                M5.Display.width(),
                                itemHeight,
                                SELECTED_BG);
        }
        else
        {
            M5.Display.setTextColor(NORMAL_FG, NORMAL_BG);
        }

        // Draw label with padding
        M5.Display.setCursor(SELECTION_MARGIN, y + ITEM_PADDING + ceil((ITEM_HEIGHT - M5.Display.fontHeight()) / 2));
        M5.Display.print(item.label.c_str());

        // Draw description if present
        if (!item.description.empty())
        {
            if (!item.enabled)
            {
                M5.Display.setTextColor(DISABLED_FG, NORMAL_BG);
            }
            else if (isSelected)
            {
                M5.Display.setTextColor(SELECTED_FG, SELECTED_BG);
            }
            else
            {
                M5.Display.setTextColor(NORMAL_FG, NORMAL_BG);
            }

            M5.Display.setCursor(SELECTION_MARGIN + DESCRIPTION_OFFSET, y + ITEM_PADDING + ITEM_HEIGHT);
            M5.Display.print(item.description.c_str());
        }

        y += itemHeight;
    }

    prevSelectedIndex = selectedIndex;
}
