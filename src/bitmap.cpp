#include "bitmap.hpp"
#include <stdexcept>

namespace screen_renderer
{

// --- bitmap_t Implementation ---

bitmap_t::bitmap_t() : m_width(0), m_height(0)
{
}

bitmap_t::bitmap_t(size_t width, size_t height) : m_width(width), m_height(height), m_pixels(width * height, false)
{
}

bitmap_t::bitmap_t(size_t width, size_t height, const std::vector<bool> &data) : m_width(width), m_height(height), m_pixels(data)
{
  if (data.size() != width * height)
  {
    throw std::invalid_argument("Bitmap data size must match width * height");
  }
}

auto bitmap_t::get_width() const -> size_t
{
  return m_width;
}

auto bitmap_t::get_height() const -> size_t
{
  return m_height;
}

auto bitmap_t::get_pixel(size_t x, size_t y) const -> bool
{
  if (x >= m_width || y >= m_height)
  {
    return false;
  }
  return m_pixels[y * m_width + x];
}

auto bitmap_t::set_pixel(size_t x, size_t y, bool value) -> void
{
  if (x >= m_width || y >= m_height)
  {
    return;
  }
  m_pixels[y * m_width + x] = value;
}

auto bitmap_t::get_data() const -> const std::vector<bool> &
{
  return m_pixels;
}

// Factory method: Smiley face (16x16)
auto bitmap_t::create_smiley() -> bitmap_t
{
  bitmap_t bmp(16, 16);

  // Draw circle outline
  const size_t center_x = 8;
  const size_t center_y = 8;
  const size_t radius = 7;

  for (size_t y = 0; y < 16; ++y)
  {
    for (size_t x = 0; x < 16; ++x)
    {
      int dx = static_cast<int>(x) - static_cast<int>(center_x);
      int dy = static_cast<int>(y) - static_cast<int>(center_y);
      int dist_sq = dx * dx + dy * dy;

      // Circle outline
      if (dist_sq >= (radius - 1) * (radius - 1) && dist_sq <= radius * radius)
      {
        bmp.set_pixel(x, y, true);
      }
    }
  }

  // Eyes
  bmp.set_pixel(5, 6, true);
  bmp.set_pixel(11, 6, true);

  // Smile
  bmp.set_pixel(5, 11, true);
  bmp.set_pixel(6, 12, true);
  bmp.set_pixel(7, 12, true);
  bmp.set_pixel(8, 12, true);
  bmp.set_pixel(9, 12, true);
  bmp.set_pixel(10, 12, true);
  bmp.set_pixel(11, 11, true);

  return bmp;
}

// Factory method: Heart (16x16)
auto bitmap_t::create_heart() -> bitmap_t
{
  bitmap_t bmp(16, 16);

  // Heart shape pattern
  std::vector<std::vector<int>> pattern = {
      {0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
      {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0}, {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  };

  for (size_t y = 0; y < pattern.size() && y < 16; ++y)
  {
    for (size_t x = 0; x < pattern[y].size() && x < 16; ++x)
    {
      if (pattern[y][x] == 1)
      {
        bmp.set_pixel(x, y + 2, true);
      }
    }
  }

  return bmp;
}

// Factory method: Arrow Up (12x12)
auto bitmap_t::create_arrow_up() -> bitmap_t
{
  bitmap_t bmp(12, 12);

  // Arrow pointing up
  bmp.set_pixel(6, 1, true);
  bmp.set_pixel(5, 2, true);
  bmp.set_pixel(6, 2, true);
  bmp.set_pixel(7, 2, true);
  bmp.set_pixel(4, 3, true);
  bmp.set_pixel(5, 3, true);
  bmp.set_pixel(6, 3, true);
  bmp.set_pixel(7, 3, true);
  bmp.set_pixel(8, 3, true);
  bmp.set_pixel(3, 4, true);
  bmp.set_pixel(4, 4, true);
  bmp.set_pixel(5, 4, true);
  bmp.set_pixel(6, 4, true);
  bmp.set_pixel(7, 4, true);
  bmp.set_pixel(8, 4, true);
  bmp.set_pixel(9, 4, true);

  // Shaft
  for (size_t y = 5; y < 11; ++y)
  {
    bmp.set_pixel(5, y, true);
    bmp.set_pixel(6, y, true);
    bmp.set_pixel(7, y, true);
  }

  return bmp;
}

// Factory method: Arrow Down (12x12)
auto bitmap_t::create_arrow_down() -> bitmap_t
{
  bitmap_t bmp(12, 12);

  // Shaft
  for (size_t y = 1; y < 7; ++y)
  {
    bmp.set_pixel(5, y, true);
    bmp.set_pixel(6, y, true);
    bmp.set_pixel(7, y, true);
  }

  // Arrow head
  bmp.set_pixel(3, 7, true);
  bmp.set_pixel(4, 7, true);
  bmp.set_pixel(5, 7, true);
  bmp.set_pixel(6, 7, true);
  bmp.set_pixel(7, 7, true);
  bmp.set_pixel(8, 7, true);
  bmp.set_pixel(9, 7, true);
  bmp.set_pixel(4, 8, true);
  bmp.set_pixel(5, 8, true);
  bmp.set_pixel(6, 8, true);
  bmp.set_pixel(7, 8, true);
  bmp.set_pixel(8, 8, true);
  bmp.set_pixel(5, 9, true);
  bmp.set_pixel(6, 9, true);
  bmp.set_pixel(7, 9, true);
  bmp.set_pixel(6, 10, true);

  return bmp;
}

// Factory method: Arrow Left (12x12)
auto bitmap_t::create_arrow_left() -> bitmap_t
{
  bitmap_t bmp(12, 12);

  // Arrow head
  bmp.set_pixel(1, 6, true);
  bmp.set_pixel(2, 5, true);
  bmp.set_pixel(2, 6, true);
  bmp.set_pixel(2, 7, true);
  bmp.set_pixel(3, 4, true);
  bmp.set_pixel(3, 5, true);
  bmp.set_pixel(3, 6, true);
  bmp.set_pixel(3, 7, true);
  bmp.set_pixel(3, 8, true);
  bmp.set_pixel(4, 3, true);
  bmp.set_pixel(4, 4, true);
  bmp.set_pixel(4, 5, true);
  bmp.set_pixel(4, 6, true);
  bmp.set_pixel(4, 7, true);
  bmp.set_pixel(4, 8, true);
  bmp.set_pixel(4, 9, true);

  // Shaft
  for (size_t x = 5; x < 11; ++x)
  {
    bmp.set_pixel(x, 5, true);
    bmp.set_pixel(x, 6, true);
    bmp.set_pixel(x, 7, true);
  }

  return bmp;
}

// Factory method: Arrow Right (12x12)
auto bitmap_t::create_arrow_right() -> bitmap_t
{
  bitmap_t bmp(12, 12);

  // Shaft
  for (size_t x = 1; x < 7; ++x)
  {
    bmp.set_pixel(x, 5, true);
    bmp.set_pixel(x, 6, true);
    bmp.set_pixel(x, 7, true);
  }

  // Arrow head
  bmp.set_pixel(7, 3, true);
  bmp.set_pixel(7, 4, true);
  bmp.set_pixel(7, 5, true);
  bmp.set_pixel(7, 6, true);
  bmp.set_pixel(7, 7, true);
  bmp.set_pixel(7, 8, true);
  bmp.set_pixel(7, 9, true);
  bmp.set_pixel(8, 4, true);
  bmp.set_pixel(8, 5, true);
  bmp.set_pixel(8, 6, true);
  bmp.set_pixel(8, 7, true);
  bmp.set_pixel(8, 8, true);
  bmp.set_pixel(9, 5, true);
  bmp.set_pixel(9, 6, true);
  bmp.set_pixel(9, 7, true);
  bmp.set_pixel(10, 6, true);

  return bmp;
}

// Factory method: Checkmark (12x12)
auto bitmap_t::create_checkmark() -> bitmap_t
{
  bitmap_t bmp(12, 12);

  // Checkmark pattern
  bmp.set_pixel(9, 2, true);
  bmp.set_pixel(8, 3, true);
  bmp.set_pixel(9, 3, true);
  bmp.set_pixel(7, 4, true);
  bmp.set_pixel(8, 4, true);
  bmp.set_pixel(4, 5, true);
  bmp.set_pixel(6, 5, true);
  bmp.set_pixel(7, 5, true);
  bmp.set_pixel(3, 6, true);
  bmp.set_pixel(4, 6, true);
  bmp.set_pixel(6, 6, true);
  bmp.set_pixel(2, 7, true);
  bmp.set_pixel(3, 7, true);
  bmp.set_pixel(5, 7, true);
  bmp.set_pixel(2, 8, true);
  bmp.set_pixel(4, 8, true);
  bmp.set_pixel(3, 9, true);

  return bmp;
}

// Factory method: Cross (12x12)
auto bitmap_t::create_cross() -> bitmap_t
{
  bitmap_t bmp(12, 12);

  // Cross/X pattern
  for (size_t i = 0; i < 10; ++i)
  {
    bmp.set_pixel(2 + i, 2 + i, true);
    bmp.set_pixel(11 - i, 2 + i, true);
  }

  return bmp;
}

auto bitmap_t::create_drone() -> bitmap_t
{
  bitmap_t bmp(14, 7);
  // Center body
  bmp.set_pixel(6, 3, true);
  bmp.set_pixel(7, 3, true);
  // Arms
  bmp.set_pixel(4, 2, true);
  bmp.set_pixel(5, 3, true);
  bmp.set_pixel(8, 3, true);
  bmp.set_pixel(9, 2, true);
  bmp.set_pixel(4, 4, true);
  bmp.set_pixel(9, 4, true);
  // Rotors
  bmp.set_pixel(3, 1, true);
  bmp.set_pixel(4, 1, true);
  bmp.set_pixel(5, 1, true);
  bmp.set_pixel(10, 1, true);
  bmp.set_pixel(11, 1, true);
  bmp.set_pixel(9, 1, true);
  bmp.set_pixel(3, 5, true);
  bmp.set_pixel(4, 5, true);
  bmp.set_pixel(5, 5, true);
  bmp.set_pixel(10, 5, true);
  bmp.set_pixel(11, 5, true);
  bmp.set_pixel(9, 5, true);
  return bmp;
}

auto bitmap_t::create_battery() -> bitmap_t
{
  bitmap_t bmp(12, 7);
  // Body outline
  for (int x = 0; x < 10; ++x)
  {
    bmp.set_pixel(x, 0, true);
    bmp.set_pixel(x, 6, true);
  }
  for (int y = 0; y < 7; ++y)
  {
    bmp.set_pixel(0, y, true);
    bmp.set_pixel(10, y, true);
  }
  // Terminal
  bmp.set_pixel(11, 2, true);
  bmp.set_pixel(11, 3, true);
  bmp.set_pixel(11, 4, true);
  // Fill 80% (fixed for icon)
  for (int x = 2; x < 8; ++x)
  {
    for (int y = 2; y < 5; ++y)
    {
      bmp.set_pixel(x, y, true);
    }
  }
  return bmp;
}

} // namespace screen_renderer
