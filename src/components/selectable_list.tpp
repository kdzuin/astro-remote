#pragma once

template <typename IdType>
SelectableList<IdType>::SelectableList() : title("Menu") {}

template <typename IdType>
SelectableList<IdType>::SelectableList(const std::string &customTitle) : title(customTitle) {}

template <typename IdType>
void SelectableList<IdType>::clear()
{
    items.clear();
    selectedIndex = 0;
}

template <typename IdType>
void SelectableList<IdType>::addItem(IdType id, const std::string &label, bool enabled)
{
    items.emplace_back(id, label, enabled);
}

template <typename IdType>
void SelectableList<IdType>::addItem(IdType id, const std::string &label, const std::string &infoText, uint16_t infoColor, bool enabled)
{
    items.emplace_back(id, label, infoText, infoColor, enabled);
}

template <typename IdType>
void SelectableList<IdType>::addItem(IdType id, const std::string &label, const std::string &infoText, bool enabled)
{
    items.emplace_back(id, label, infoText, enabled);
}

template <typename IdType>
void SelectableList<IdType>::setTitle(const std::string &newTitle)
{
    title = newTitle;
}

template <typename IdType>
void SelectableList<IdType>::setSelectedIndex(int index)
{
    if (index >= 0 && index < items.size())
    {
        selectedIndex = index;
    }
}

template <typename IdType>
IdType SelectableList<IdType>::getSelectedId() const
{
    return items.empty() ? IdType{} : items[selectedIndex].id;
}

template <typename IdType>
const typename SelectableList<IdType>::Item &SelectableList<IdType>::getSelectedItem() const
{
    return items[selectedIndex];
}

template <typename IdType>
const char *SelectableList<IdType>::getItem(int index) const
{
    return items[index].label.c_str();
}

template <typename IdType>
bool SelectableList<IdType>::isEmpty() const
{
    return items.empty();
}

template <typename IdType>
size_t SelectableList<IdType>::size() const
{
    return items.size();
}

template <typename IdType>
bool SelectableList<IdType>::selectNext()
{
    if (items.empty())
    {
        return false;
    }

    size_t startIndex = selectedIndex;
    do
    {
        selectedIndex = (selectedIndex + 1) % items.size();
        if (items[selectedIndex].enabled)
        {
            return true;
        }
    } while (selectedIndex != startIndex);

    // If we got here, try to find any enabled item
    for (size_t i = 0; i < items.size(); i++)
    {
        if (items[i].enabled)
        {
            selectedIndex = i;
            return true;
        }
    }

    return false; // No enabled items found
}

template <typename IdType>
int SelectableList<IdType>::getSelectedIndex() const
{
    return selectedIndex;
}

template <typename IdType>
void SelectableList<IdType>::draw()
{
    int y = 0;

    M5.Display.fillScreen(BLACK);
    M5.Display.setTextSize(1.25);

    // Draw title
    M5.Display.setTextColor(NORMAL_FG);
    M5.Display.setTextDatum(middle_left);
    M5.Display.drawString(title.c_str(), HORIZONTAL_PADDING, y + ITEM_HEIGHT / 2);
    y += ITEM_HEIGHT + ITEM_PADDING + TITLE_PADDING;

    // Draw separator line
    M5.Display.drawLine(0, y - 4, M5.Display.width(), y - 4, NORMAL_FG);

    // Draw all items
    for (size_t i = 0; i < items.size(); i++)
    {
        const auto &item = items[i];
        bool isSelected = (i == selectedIndex);
        int itemHeight = ITEM_HEIGHT;

        // Draw selection background
        if (isSelected)
        {
            M5.Display.fillRect(0, y, M5.Display.width(), ITEM_HEIGHT, SELECTED_BG);
        }

        // Draw item text
        M5.Display.setTextDatum(middle_left);
        M5.Display.setTextColor(isSelected ? (item.enabled ? SELECTED_FG : DISABLED_FG) : (item.enabled ? NORMAL_FG : DISABLED_FG));
        M5.Display.drawString(item.label.c_str(), HORIZONTAL_PADDING, y + ITEM_HEIGHT / 2);

        // Draw info section if present
        if (item.info)
        {
            if (item.info->color)
            {
                M5.Display.setTextColor(*item.info->color);
            }
            M5.Display.setTextDatum(middle_right);
            M5.Display.drawString(item.info->text.c_str(), M5.Display.width() - HORIZONTAL_PADDING, y + ITEM_HEIGHT / 2);
        }

        y += ITEM_HEIGHT + ITEM_PADDING;
    }
}
