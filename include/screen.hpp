#pragma once

#include <vector>
#include <cstddef>
#include <string>

namespace screen_renderer
{

// Forward declaration
class bitmap_t;
class font_t;

class screen_t
{
public:
  // Functions
  screen_t(size_t width = 128, size_t height = 64);
  auto get_width() const -> size_t;
  auto get_height() const -> size_t;
  auto set_pixel(size_t x, size_t y, bool value) -> void;
  auto get_pixel(size_t x, size_t y) const -> bool;
  auto clear() -> void;
  auto fill() -> void;
  auto get_data() const -> const std::vector<bool> &;

  // Bitmap and text drawing
  auto draw_bitmap(const bitmap_t &bitmap, size_t x, size_t y) -> void;
  auto draw_bitmap(const bitmap_t &bitmap, size_t x, size_t y, bool value) -> void;
  auto draw_text(const font_t &font, const std::string &text, size_t x, size_t y, size_t spacing = 1) -> void;
  auto draw_text(const font_t &font, const std::string &text, size_t x, size_t y, size_t spacing, bool value) -> void;

  // Drawing primitives
  auto draw_line(int x0, int y0, int x1, int y1, bool value) -> void;
  auto draw_rect(int x, int y, int width, int height, bool value) -> void;

private:
  // Variables
  size_t m_width;
  size_t m_height;
  std::vector<bool> m_pixels;
};

} // namespace screen_renderer
