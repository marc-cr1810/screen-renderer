#pragma once

#include <vector>
#include <cstddef>

namespace screen_renderer
{

class bitmap_t
{
public:
  // Functions
  bitmap_t(size_t width, size_t height);
  bitmap_t(size_t width, size_t height, const std::vector<bool> &data);
  auto get_width() const -> size_t;
  auto get_height() const -> size_t;
  auto get_pixel(size_t x, size_t y) const -> bool;
  auto set_pixel(size_t x, size_t y, bool value) -> void;
  auto get_data() const -> const std::vector<bool> &;

  // Factory methods for common test bitmaps
  static auto create_smiley() -> bitmap_t;
  static auto create_heart() -> bitmap_t;
  static auto create_arrow_up() -> bitmap_t;
  static auto create_arrow_down() -> bitmap_t;
  static auto create_arrow_left() -> bitmap_t;
  static auto create_arrow_right() -> bitmap_t;
  static auto create_checkmark() -> bitmap_t;
  static auto create_cross() -> bitmap_t;

private:
  // Variables
  size_t m_width;
  size_t m_height;
  std::vector<bool> m_pixels;
};

} // namespace screen_renderer
