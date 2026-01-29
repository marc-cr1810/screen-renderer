#pragma once

#include "bitmap.hpp"
#include <cstddef>
#include <string>

namespace screen_renderer
{

class font_t
{
public:
  // Functions
  font_t();
  auto get_character(char c) const -> bitmap_t;
  auto get_char_width() const -> size_t;
  auto get_char_height() const -> size_t;

private:
  // Variables
  static constexpr size_t CHAR_WIDTH = 5;
  static constexpr size_t CHAR_HEIGHT = 7;

  // Font data storage (96 printable ASCII characters)
  auto get_char_data(char c) const -> const unsigned char *;
};

} // namespace screen_renderer
