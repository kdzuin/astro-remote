#pragma once

#include <M5Unified.h>

struct RGBColorTriple {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

namespace colors {

/**
 * Convert RGB color triple to M5.Display color format
 */
inline uint32_t fromRGB(const RGBColorTriple& color) {
    return M5.Display.color888(color.r, color.g, color.b);
}

/**
 * Convert RGB values to M5.Display color format
 */
inline uint32_t fromRGB(uint8_t r, uint8_t g, uint8_t b) {
    return M5.Display.color888(r, g, b);
}

/**
 * Get color from the predefined palette
 */
inline uint32_t get(const RGBColorTriple& color) {
    return fromRGB(color);
}

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

// blues
constexpr RGBColorTriple BLUE_50 = {239, 246, 255};
constexpr RGBColorTriple BLUE_100 = {219, 234, 254};
constexpr RGBColorTriple BLUE_200 = {191, 219, 254};
constexpr RGBColorTriple BLUE_300 = {147, 197, 253};
constexpr RGBColorTriple BLUE_400 = {96, 165, 250};
constexpr RGBColorTriple BLUE_500 = {59, 130, 246};
constexpr RGBColorTriple BLUE_600 = {37, 99, 235};
constexpr RGBColorTriple BLUE_700 = {29, 78, 216};
constexpr RGBColorTriple BLUE_800 = {30, 64, 175};
constexpr RGBColorTriple BLUE_900 = {30, 58, 138};
constexpr RGBColorTriple BLUE_950 = {23, 37, 84};

// greens
constexpr RGBColorTriple GREEN_50 = {240, 253, 244};
constexpr RGBColorTriple GREEN_100 = {220, 252, 231};
constexpr RGBColorTriple GREEN_200 = {187, 247, 208};
constexpr RGBColorTriple GREEN_300 = {134, 239, 172};
constexpr RGBColorTriple GREEN_400 = {74, 222, 128};
constexpr RGBColorTriple GREEN_500 = {34, 197, 94};
constexpr RGBColorTriple GREEN_600 = {22, 163, 74};
constexpr RGBColorTriple GREEN_700 = {21, 128, 61};
constexpr RGBColorTriple GREEN_800 = {22, 101, 52};
constexpr RGBColorTriple GREEN_900 = {20, 83, 45};
constexpr RGBColorTriple GREEN_950 = {5, 46, 22};

// yellows
constexpr RGBColorTriple YELLOW_50 = {254, 252, 232};
constexpr RGBColorTriple YELLOW_100 = {254, 249, 195};
constexpr RGBColorTriple YELLOW_200 = {254, 240, 138};
constexpr RGBColorTriple YELLOW_300 = {253, 224, 71};
constexpr RGBColorTriple YELLOW_400 = {250, 204, 21};
constexpr RGBColorTriple YELLOW_500 = {234, 179, 8};
constexpr RGBColorTriple YELLOW_600 = {202, 138, 4};
constexpr RGBColorTriple YELLOW_700 = {161, 98, 7};
constexpr RGBColorTriple YELLOW_800 = {133, 77, 14};
constexpr RGBColorTriple YELLOW_900 = {113, 63, 18};
constexpr RGBColorTriple YELLOW_950 = {66, 32, 6};

// oranges
constexpr RGBColorTriple ORANGE_50 = {255, 247, 237};
constexpr RGBColorTriple ORANGE_100 = {255, 237, 213};
constexpr RGBColorTriple ORANGE_200 = {254, 215, 170};
constexpr RGBColorTriple ORANGE_300 = {253, 186, 116};
constexpr RGBColorTriple ORANGE_400 = {251, 146, 60};
constexpr RGBColorTriple ORANGE_500 = {249, 115, 22};
constexpr RGBColorTriple ORANGE_600 = {234, 88, 12};
constexpr RGBColorTriple ORANGE_700 = {194, 65, 12};
constexpr RGBColorTriple ORANGE_800 = {154, 52, 18};
constexpr RGBColorTriple ORANGE_900 = {124, 45, 18};
constexpr RGBColorTriple ORANGE_950 = {67, 20, 7};

// reds
constexpr RGBColorTriple RED_50 = {254, 242, 242};
constexpr RGBColorTriple RED_100 = {254, 226, 226};
constexpr RGBColorTriple RED_200 = {254, 202, 202};
constexpr RGBColorTriple RED_300 = {252, 165, 165};
constexpr RGBColorTriple RED_400 = {248, 113, 113};
constexpr RGBColorTriple RED_500 = {239, 68, 68};
constexpr RGBColorTriple RED_600 = {220, 38, 38};
constexpr RGBColorTriple RED_700 = {185, 28, 28};
constexpr RGBColorTriple RED_800 = {153, 27, 27};
constexpr RGBColorTriple RED_900 = {127, 29, 29};
constexpr RGBColorTriple RED_950 = {69, 10, 10};

// Additional color families
constexpr RGBColorTriple INDIGO_50 = {238, 242, 255};
constexpr RGBColorTriple INDIGO_100 = {224, 231, 255};
constexpr RGBColorTriple INDIGO_200 = {199, 210, 254};
constexpr RGBColorTriple INDIGO_300 = {165, 180, 252};
constexpr RGBColorTriple INDIGO_400 = {129, 140, 248};
constexpr RGBColorTriple INDIGO_500 = {99, 102, 241};
constexpr RGBColorTriple INDIGO_600 = {79, 70, 229};
constexpr RGBColorTriple INDIGO_700 = {67, 56, 202};
constexpr RGBColorTriple INDIGO_800 = {55, 48, 163};
constexpr RGBColorTriple INDIGO_900 = {49, 46, 129};
constexpr RGBColorTriple INDIGO_950 = {30, 27, 75};

constexpr RGBColorTriple PURPLE_50 = {250, 245, 255};
constexpr RGBColorTriple PURPLE_100 = {243, 232, 255};
constexpr RGBColorTriple PURPLE_200 = {233, 213, 255};
constexpr RGBColorTriple PURPLE_300 = {216, 180, 254};
constexpr RGBColorTriple PURPLE_400 = {192, 132, 252};
constexpr RGBColorTriple PURPLE_500 = {168, 85, 247};
constexpr RGBColorTriple PURPLE_600 = {147, 51, 234};
constexpr RGBColorTriple PURPLE_700 = {126, 34, 206};
constexpr RGBColorTriple PURPLE_800 = {107, 33, 168};
constexpr RGBColorTriple PURPLE_900 = {88, 28, 135};
constexpr RGBColorTriple PURPLE_950 = {59, 7, 100};

constexpr RGBColorTriple PINK_50 = {253, 242, 248};
constexpr RGBColorTriple PINK_100 = {252, 231, 243};
constexpr RGBColorTriple PINK_200 = {251, 207, 232};
constexpr RGBColorTriple PINK_300 = {249, 168, 212};
constexpr RGBColorTriple PINK_400 = {244, 114, 182};
constexpr RGBColorTriple PINK_500 = {236, 72, 153};
constexpr RGBColorTriple PINK_600 = {219, 39, 119};
constexpr RGBColorTriple PINK_700 = {190, 24, 93};
constexpr RGBColorTriple PINK_800 = {157, 23, 77};
constexpr RGBColorTriple PINK_900 = {131, 24, 67};
constexpr RGBColorTriple PINK_950 = {80, 7, 36};

constexpr RGBColorTriple ROSE_50 = {255, 241, 242};
constexpr RGBColorTriple ROSE_100 = {255, 228, 230};
constexpr RGBColorTriple ROSE_200 = {254, 205, 211};
constexpr RGBColorTriple ROSE_300 = {253, 164, 175};
constexpr RGBColorTriple ROSE_400 = {251, 113, 133};
constexpr RGBColorTriple ROSE_500 = {244, 63, 94};
constexpr RGBColorTriple ROSE_600 = {225, 29, 72};
constexpr RGBColorTriple ROSE_700 = {190, 18, 60};
constexpr RGBColorTriple ROSE_800 = {159, 18, 57};
constexpr RGBColorTriple ROSE_900 = {136, 19, 55};
constexpr RGBColorTriple ROSE_950 = {76, 5, 25};

constexpr RGBColorTriple EMERALD_50 = {236, 253, 245};
constexpr RGBColorTriple EMERALD_100 = {209, 250, 229};
constexpr RGBColorTriple EMERALD_200 = {167, 243, 208};
constexpr RGBColorTriple EMERALD_300 = {110, 231, 183};
constexpr RGBColorTriple EMERALD_400 = {52, 211, 153};
constexpr RGBColorTriple EMERALD_500 = {16, 185, 129};
constexpr RGBColorTriple EMERALD_600 = {5, 150, 105};
constexpr RGBColorTriple EMERALD_700 = {4, 120, 87};
constexpr RGBColorTriple EMERALD_800 = {6, 95, 70};
constexpr RGBColorTriple EMERALD_900 = {6, 78, 59};
constexpr RGBColorTriple EMERALD_950 = {2, 44, 34};

constexpr RGBColorTriple TEAL_50 = {240, 253, 250};
constexpr RGBColorTriple TEAL_100 = {204, 251, 241};
constexpr RGBColorTriple TEAL_200 = {153, 246, 228};
constexpr RGBColorTriple TEAL_300 = {94, 234, 212};
constexpr RGBColorTriple TEAL_400 = {45, 212, 191};
constexpr RGBColorTriple TEAL_500 = {20, 184, 166};
constexpr RGBColorTriple TEAL_600 = {13, 148, 136};
constexpr RGBColorTriple TEAL_700 = {15, 118, 110};
constexpr RGBColorTriple TEAL_800 = {17, 94, 89};
constexpr RGBColorTriple TEAL_900 = {19, 78, 74};
constexpr RGBColorTriple TEAL_950 = {4, 47, 46};

constexpr RGBColorTriple CYAN_50 = {236, 254, 255};
constexpr RGBColorTriple CYAN_100 = {207, 250, 254};
constexpr RGBColorTriple CYAN_200 = {165, 243, 252};
constexpr RGBColorTriple CYAN_300 = {103, 232, 249};
constexpr RGBColorTriple CYAN_400 = {34, 211, 238};
constexpr RGBColorTriple CYAN_500 = {6, 182, 212};
constexpr RGBColorTriple CYAN_600 = {8, 145, 178};
constexpr RGBColorTriple CYAN_700 = {14, 116, 144};
constexpr RGBColorTriple CYAN_800 = {21, 94, 117};
constexpr RGBColorTriple CYAN_900 = {22, 78, 99};
constexpr RGBColorTriple CYAN_950 = {8, 51, 68};

constexpr RGBColorTriple SKY_50 = {240, 249, 255};
constexpr RGBColorTriple SKY_100 = {224, 242, 254};
constexpr RGBColorTriple SKY_200 = {186, 230, 253};
constexpr RGBColorTriple SKY_300 = {125, 211, 252};
constexpr RGBColorTriple SKY_400 = {56, 189, 248};
constexpr RGBColorTriple SKY_500 = {14, 165, 233};
constexpr RGBColorTriple SKY_600 = {2, 132, 199};
constexpr RGBColorTriple SKY_700 = {3, 105, 161};
constexpr RGBColorTriple SKY_800 = {7, 89, 133};
constexpr RGBColorTriple SKY_900 = {12, 74, 110};
constexpr RGBColorTriple SKY_950 = {8, 47, 73};

constexpr RGBColorTriple SLATE_50 = {248, 250, 252};
constexpr RGBColorTriple SLATE_100 = {241, 245, 249};
constexpr RGBColorTriple SLATE_200 = {226, 232, 240};
constexpr RGBColorTriple SLATE_300 = {203, 213, 225};
constexpr RGBColorTriple SLATE_400 = {148, 163, 184};
constexpr RGBColorTriple SLATE_500 = {100, 116, 139};
constexpr RGBColorTriple SLATE_600 = {71, 85, 105};
constexpr RGBColorTriple SLATE_700 = {51, 65, 85};
constexpr RGBColorTriple SLATE_800 = {30, 41, 59};
constexpr RGBColorTriple SLATE_900 = {15, 23, 42};
constexpr RGBColorTriple SLATE_950 = {2, 6, 23};

}  // namespace colors
