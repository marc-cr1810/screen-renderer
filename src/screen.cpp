#include "screen.hpp"
#include "bitmap.hpp"
#include "font.hpp"
#include <cmath>

namespace screen_renderer
{

screen_t::screen_t(size_t width, size_t height) : m_width(width), m_height(height), m_pixels(width * height, false)
{
}

auto screen_t::get_width() const -> size_t
{
  return m_width;
}

auto screen_t::get_height() const -> size_t
{
  return m_height;
}

auto screen_t::set_pixel(size_t x, size_t y, bool value) -> void
{
  if (x < m_width && y < m_height)
  {
    m_pixels[y * m_width + x] = value;
  }
}

auto screen_t::get_pixel(size_t x, size_t y) const -> bool
{
  if (x < m_width && y < m_height)
  {
    return m_pixels[y * m_width + x];
  }
  return false;
}

auto screen_t::clear() -> void
{
  std::fill(m_pixels.begin(), m_pixels.end(), false);
}

auto screen_t::fill() -> void
{
  std::fill(m_pixels.begin(), m_pixels.end(), true);
}

auto screen_t::get_data() const -> const std::vector<bool> &
{
  return m_pixels;
}

auto screen_t::draw_bitmap(const bitmap_t &bitmap, size_t x, size_t y) -> void
{
  draw_bitmap(bitmap, x, y, true);
}

auto screen_t::draw_bitmap(const bitmap_t &bitmap, size_t x, size_t y, bool value) -> void
{
  for (size_t by = 0; by < bitmap.get_height(); ++by)
  {
    for (size_t bx = 0; bx < bitmap.get_width(); ++bx)
    {
      size_t screen_x = x + bx;
      size_t screen_y = y + by;

      // Bounds checking
      if (screen_x < m_width && screen_y < m_height)
      {
        if (bitmap.get_pixel(bx, by))
        {
          set_pixel(screen_x, screen_y, value);
        }
      }
    }
  }
}

auto screen_t::draw_text(const font_t &font, const std::string &text, size_t x, size_t y, size_t spacing) -> void
{
  draw_text(font, text, x, y, spacing, true);
}

auto screen_t::draw_text(const font_t &font, const std::string &text, size_t x, size_t y, size_t spacing, bool value) -> void
{
  size_t cursor_x = x;

  for (char c : text)
  {
    bitmap_t char_bitmap = font.get_character(c);
    draw_bitmap(char_bitmap, cursor_x, y, value);
    cursor_x += char_bitmap.get_width() + spacing;
  }
}

// Bresenham's line algorithm
auto screen_t::draw_line(int x0, int y0, int x1, int y1, bool value) -> void
{
  int dx = std::abs(x1 - x0);
  int dy = std::abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (true)
  {
    if (x0 >= 0 && x0 < static_cast<int>(m_width) && y0 >= 0 && y0 < static_cast<int>(m_height))
    {
      set_pixel(static_cast<size_t>(x0), static_cast<size_t>(y0), value);
    }

    if (x0 == x1 && y0 == y1)
      break;

    int e2 = 2 * err;
    if (e2 > -dy)
    {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

// Draw rectangle outline or filled
auto screen_t::draw_rect(int x, int y, int width, int height, bool value) -> void
{
  // Draw top and bottom edges
  for (int i = 0; i < width; ++i)
  {
    if (x + i >= 0 && x + i < static_cast<int>(m_width))
    {
      if (y >= 0 && y < static_cast<int>(m_height))
        set_pixel(static_cast<size_t>(x + i), static_cast<size_t>(y), value);
      if (y + height - 1 >= 0 && y + height - 1 < static_cast<int>(m_height))
        set_pixel(static_cast<size_t>(x + i), static_cast<size_t>(y + height - 1), value);
    }
  }

  // Draw left and right edges
  for (int i = 0; i < height; ++i)
  {
    if (y + i >= 0 && y + i < static_cast<int>(m_height))
    {
      if (x >= 0 && x < static_cast<int>(m_width))
        set_pixel(static_cast<size_t>(x), static_cast<size_t>(y + i), value);
      if (x + width - 1 >= 0 && x + width - 1 < static_cast<int>(m_width))
        set_pixel(static_cast<size_t>(x + width - 1), static_cast<size_t>(y + i), value);
    }
  }
}

} // namespace screen_renderer
