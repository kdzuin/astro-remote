#pragma once

#include "hardware_interface.h"

namespace display {
namespace colors {

constexpr RGBColorTriple NORMAL = {30, 30, 30};
constexpr RGBColorTriple SUCCESS = {0, 200, 0};
constexpr RGBColorTriple ERROR = {200, 0, 0};
constexpr RGBColorTriple WARNING = {200, 200, 0};
constexpr RGBColorTriple DANGER = {200, 200, 0};
constexpr RGBColorTriple IN_PROGRESS = {0, 50, 0};  // faded green

constexpr RGBColorTriple WHITE = {255, 255, 255};
constexpr RGBColorTriple BLACK = {0, 0, 0};

constexpr RGBColorTriple RED = {200, 0, 0};
constexpr RGBColorTriple GREEN = {0, 200, 0};
constexpr RGBColorTriple BLUE = {0, 0, 200};
constexpr RGBColorTriple YELLOW = {200, 200, 0};
constexpr RGBColorTriple CYAN = {0, 200, 200};
constexpr RGBColorTriple MAGENTA = {200, 0, 200};

constexpr RGBColorTriple GRAY_0 = {255, 255, 255};    // #ffffff
constexpr RGBColorTriple GRAY_50 = {252, 252, 252};   // #fcfcfc
constexpr RGBColorTriple GRAY_100 = {243, 243, 243};  // #f3f3f3
constexpr RGBColorTriple GRAY_200 = {224, 224, 224};  // #e0e0e0
constexpr RGBColorTriple GRAY_300 = {189, 189, 189};  // #bdbdbd
constexpr RGBColorTriple GRAY_400 = {158, 158, 158};  // #9e9e9e
constexpr RGBColorTriple GRAY_500 = {118, 118, 118};  // #767676
constexpr RGBColorTriple GRAY_600 = {79, 79, 79};     // #4f4f4f
constexpr RGBColorTriple GRAY_700 = {74, 74, 74};     // #4a4a4a
constexpr RGBColorTriple GRAY_800 = {51, 51, 51};     // #333333
constexpr RGBColorTriple GRAY_900 = {0, 0, 0};
}  // namespace colors
}  // namespace display
