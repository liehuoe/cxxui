#pragma once
#include <cstdint>

namespace cxxui {

struct Color {
    Color() = default;
    Color(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha = 255)
        : red(red),
          green(green),
          blue(blue),
          alpha(alpha) {}
    std::uint8_t red = 0;
    std::uint8_t green = 0;
    std::uint8_t blue = 0;
    std::uint8_t alpha = 0;
};

}  // namespace cxxui