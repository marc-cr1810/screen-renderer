#include "markup_renderer.hpp"
#include <iostream>
#include <algorithm>

namespace screen_renderer
{

markup_renderer_t::markup_renderer_t()
{
}

auto markup_renderer_t::load_layout(const std::string &filename) -> bool
{
  m_root = simple_markup_parser_t::parse_file(filename);
  if (!m_root)
  {
    std::cerr << "Failed to parse markup file: " << filename << std::endl;
    return false;
  }

  m_id_map.clear();
  m_visibility_map.clear();
  m_text_map.clear();
  build_id_map(m_root);
  return true;
}

auto markup_renderer_t::load_layout_from_string(const std::string &xml_content) -> bool
{
  std::vector<parse_error_t> errors;
  return load_layout_from_string(xml_content, errors);
}

auto markup_renderer_t::load_layout_from_string(const std::string &xml_content, std::vector<parse_error_t> &out_errors) -> bool
{
  m_root = simple_markup_parser_t::parse(xml_content, out_errors);
  if (!m_root)
  {
    return false;
  }

  m_id_map.clear();
  m_visibility_map.clear();
  m_text_map.clear();
  build_id_map(m_root);
  return true;
}

// Helper to find element recursively
auto find_element_by_line(std::shared_ptr<element_t> element, int line) -> std::shared_ptr<element_t>
{
  if (!element)
  {
    return nullptr;
  }

  // Check if line is within this element's range
  // Note: we want the most specific (deepest) child that contains the line.
  // But children might not cover the entire range of the parent (e.g. text content).

  if (line >= element->get_start_line() && line <= element->get_end_line())
  {
    // Check children first
    for (auto &child : element->get_children())
    {
      auto found = find_element_by_line(child, line);
      if (found)
      {
        return found;
      }
    }
    return element;
  }
  return nullptr;
}

auto markup_renderer_t::get_element_at_line(int line) -> std::shared_ptr<element_t>
{
  return find_element_by_line(m_root, line);
}

// Helper to check if point is inside element
static auto is_point_in_element(std::shared_ptr<element_t> element, int px, int py) -> bool
{
  int x = element->get_int_attribute("x", 0);
  int y = element->get_int_attribute("y", 0);

  if (element->get_name() == "rect")
  {
    int w = element->get_int_attribute("w", 0);
    int h = element->get_int_attribute("h", 0);
    return px >= x && px < x + w && py >= y && py < y + h;
  }
  else if (element->get_name() == "circle")
  {
    int cx = element->get_int_attribute("cx", 0);
    int cy = element->get_int_attribute("cy", 0);
    int r = element->get_int_attribute("r", 0);
    int dx = px - cx;
    int dy = py - cy;
    return (dx * dx + dy * dy) <= (r * r);
  }
  else if (element->get_name() == "line")
  {
    // Simple bounding box for line? Or distance?
    // Let's use bounding box for now
    int x1 = element->get_int_attribute("x1", 0);
    int y1 = element->get_int_attribute("y1", 0);
    int x2 = element->get_int_attribute("x2", 0);
    int y2 = element->get_int_attribute("y2", 0);
    int lx = std::min(x1, x2);
    int ly = std::min(y1, y2);
    int lw = std::abs(x2 - x1);
    int lh = std::abs(y2 - y1);
    // Add some tolerance
    return px >= lx - 2 && px <= lx + lw + 2 && py >= ly - 2 && py <= ly + lh + 2;
  }
  else if (element->get_name() == "text")
  {
    int w = element->get_text_content().length() * 6 * element->get_int_attribute("scale", 1);
    int h = 8 * element->get_int_attribute("scale", 1);
    return px >= x && px < x + w && py >= y && py < y + h;
  }
  return false;
}

// Helper to find element by position recursively
auto find_element_by_pos(std::shared_ptr<element_t> element, int x, int y) -> std::shared_ptr<element_t>
{
  if (!element)
  {
    return nullptr;
  }

  // Check children first (reverse order to hit top-most)
  const auto &children = element->get_children();
  for (auto it = children.rbegin(); it != children.rend(); ++it)
  {
    auto found = find_element_by_pos(*it, x, y);
    if (found)
    {
      return found;
    }
  }

  // Check self
  if (is_point_in_element(element, x, y))
  {
    return element;
  }

  return nullptr;
}

auto markup_renderer_t::get_element_at_pos(int x, int y) -> std::shared_ptr<element_t>
{
  return find_element_by_pos(m_root, x, y);
}

auto markup_renderer_t::build_id_map(std::shared_ptr<element_t> element) -> void
{
  if (!element)
  {
    return;
  }

  std::string id = element->get_attribute("id");
  if (!id.empty())
  {
    m_id_map[id] = element;
  }

  for (auto &child : element->get_children())
  {
    build_id_map(child);
  }
}

auto markup_renderer_t::set_text(const std::string &id, const std::string &text) -> void
{
  m_text_map[id] = text;
}

auto markup_renderer_t::set_visible(const std::string &id, bool visible) -> void
{
  m_visibility_map[id] = visible;
}

auto markup_renderer_t::find_element_by_id(const std::string &id) -> std::shared_ptr<element_t>
{
  auto it = m_id_map.find(id);
  if (it != m_id_map.end())
  {
    return it->second;
  }
  return nullptr;
}

auto markup_renderer_t::register_bitmap(const std::string &id, const bitmap_t &bitmap) -> void
{
  m_bitmaps[id] = bitmap;
}

// --- Drawing Helpers ---

static void draw_filled_rect(screen_t &screen, int x, int y, int w, int h, bool color)
{
  for (int i = 0; i < h; ++i)
  {
    screen.draw_line(x, y + i, x + w - 1, y + i, color);
  }
}

static void draw_text_scaled(screen_t &screen, const font_t &font, const std::string &text, int x, int y, int scale)
{
  int cursor_x = x;
  for (char c : text)
  {
    auto bitmap = font.get_character(c);
    // Draw scaled glyph
    for (size_t row = 0; row < bitmap.get_height(); ++row)
    {
      for (size_t col = 0; col < bitmap.get_width(); ++col)
      {
        if (bitmap.get_pixel(col, row))
        {
          for (int sy = 0; sy < scale; ++sy)
          {
            for (int sx = 0; sx < scale; ++sx)
            {
              screen.set_pixel(cursor_x + col * scale + sx, y + row * scale + sy, true);
            }
          }
        }
      }
    }
    cursor_x += (bitmap.get_width() + 1) * scale;
  }
}

// Draw a large 3x5 digit scaled up by scale factor (Reused from sensor_status_demo logic)
static void draw_digit_scaled(screen_t &screen, int x, int y, int digit, int scale)
{
  static const bool digits[10][15] = {
      {1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1}, // 0
      {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0}, // 1
      {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1}, // 2
      {1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1}, // 3
      {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1}, // 4
      {1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1}, // 5
      {1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1}, // 6
      {1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1}, // 7
      {1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1}, // 8
      {1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1}, // 9
  };

  if (digit < 0 || digit > 9)
  {
    return;
  }
  const bool *d = digits[digit];
  for (int r = 0; r < 5; ++r)
  {
    for (int c = 0; c < 3; ++c)
    {
      if (d[r * 3 + c])
      {
        draw_filled_rect(screen, x + c * scale, y + r * scale, scale, scale, true);
      }
    }
  }
}

static void draw_text_big(screen_t &screen, int x, int y, const std::string &text, int scale)
{
  int cursor_x = x;
  for (char c : text)
  {
    if (c >= '0' && c <= '9')
    {
      draw_digit_scaled(screen, cursor_x, y, c - '0', scale);
      cursor_x += (3 * scale) + scale;
    }
    else
    {
      // Fallback for non-digits in "big" font - just skip or maybe standard text?
      // For now let's assume big text is just numbers as per requirement
      cursor_x += (3 * scale) + scale;
    }
  }
}

// Bresenham's Circle Algorithm
static void draw_circle(screen_t &screen, int xc, int yc, int r, bool fill)
{
  int x = 0;
  int y = r;
  int d = 3 - 2 * r;

  auto draw_circle_points = [&](int xc, int yc, int x, int y)
  {
    if (fill)
    {
      screen.draw_line(xc - x, yc + y, xc + x, yc + y, true);
      screen.draw_line(xc - x, yc - y, xc + x, yc - y, true);
      screen.draw_line(xc - y, yc + x, xc + y, yc + x, true);
      screen.draw_line(xc - y, yc - x, xc + y, yc - x, true);
    }
    else
    {
      screen.set_pixel(xc + x, yc + y, true);
      screen.set_pixel(xc - x, yc + y, true);
      screen.set_pixel(xc + x, yc - y, true);
      screen.set_pixel(xc - x, yc - y, true);
      screen.set_pixel(xc + y, yc + x, true);
      screen.set_pixel(xc - y, yc + x, true);
      screen.set_pixel(xc + y, yc - x, true);
      screen.set_pixel(xc - y, yc - x, true);
    }
  };

  while (y >= x)
  {
    draw_circle_points(xc, yc, x, y);
    x++;
    if (d > 0)
    {
      y--;
      d = d + 4 * (x - y) + 10;
    }
    else
    {
      d = d + 4 * x + 6;
    }
  }
}

auto markup_renderer_t::render(screen_t &screen) -> void
{
  if (!m_root)
  {
    return;
  }
  m_styles.clear(); // Clear styles on re-render to allow updates if parsed on fly (or just once)
  // Actually, better to parse styles in a pre-pass?
  // For now, let's collect styles during render if they are at root.
  // Or simply, when we encounter a <style>, we register it.

  render_element(screen, m_root, 0, 0);
}

auto markup_renderer_t::render_element(screen_t &screen, const std::shared_ptr<element_t> &element, int parent_x, int parent_y) -> void
{
  if (!element)
  {
    return;
  }

  // 1. Styles Definition
  if (element->get_name() == "style")
  {
    std::string id = element->get_attribute("id");
    if (!id.empty())
    {
      m_styles[id] = element->get_attributes();
      // Remove "id" from the style map itself to avoid self-reference or confusion? Not needed.
    }
    return; // Do not render style tags
  }

  // 2. Check Visibility
  std::string id = element->get_attribute("id");
  bool visible = element->get_bool_attribute("visible", true);

  // Override visibility if in map
  if (!id.empty())
  {
    auto it = m_visibility_map.find(id);
    if (it != m_visibility_map.end())
    {
      visible = it->second;
    }
  }

  if (!visible)
  {
    return;
  }

  // 3. Apply Style (if any)
  // We need to merge style attributes with element attributes.
  // Since we shouldn't modify the AST during render, we use a helper to get attribute values.
  // Helper lambda to get attribute respecting style
  auto get_attr = [&](const std::string &key, const std::string &def = "") -> std::string
  {
    // Check local attribute first
    std::string local = element->get_attribute(key);
    if (!local.empty())
      return local;

    // Check style
    std::string style_id = element->get_attribute("style");
    if (!style_id.empty() && m_styles.count(style_id))
    {
      if (m_styles[style_id].count(key))
      {
        return m_styles[style_id].at(key);
      }
    }
    return def;
  };

  auto get_int_attr = [&](const std::string &key, int def = 0) -> int
  {
    std::string val = get_attr(key);
    if (val.empty())
      return def;
    try
    {
      return std::stoi(val);
    }
    catch (...)
    {
      return def;
    }
  };

  auto get_bool_attr = [&](const std::string &key, bool def = false) -> bool
  {
    std::string val = get_attr(key);
    if (val.empty())
      return def;
    return val == "true" || val == "1" || val == "yes";
  };

  // 4. Render specific types

  // Group
  if (element->get_name() == "group")
  {
    int x = get_int_attr("x");
    int y = get_int_attr("y");

    // Render children with offset
    for (auto &child : element->get_children())
    {
      render_element(screen, child, parent_x + x, parent_y + y);
    }
    return;
  }

  if (element->get_name() == "text")
  {
    int x = parent_x + get_int_attr("x");
    int y = parent_y + get_int_attr("y");
    int scale = get_int_attr("scale", 1);

    std::string content = get_attr("text");
    if (content.empty())
    {
      content = element->get_text_content();
    }

    // Override content if in map
    if (!id.empty())
    {
      auto it = m_text_map.find(id);
      if (it != m_text_map.end())
      {
        content = it->second;
      }
    }

    if (scale > 1)
    {
      draw_text_scaled(screen, m_font, content, x, y, scale);
    }
    else
    {
      screen.draw_text(m_font, content, x, y, 1);
    }
  }
  else if (element->get_name() == "rect")
  {
    int x = parent_x + get_int_attr("x");
    int y = parent_y + get_int_attr("y");
    int w = get_int_attr("w");
    int h = get_int_attr("h");
    bool fill = get_bool_attr("fill");

    if (fill)
    {
      for (int i = 0; i < h; ++i)
      {
        screen.draw_line(x, y + i, x + w - 1, y + i, true);
      }
    }
    else
    {
      screen.draw_rect(x, y, w, h, true);
    }
  }
  else if (element->get_name() == "line")
  {
    int x1 = parent_x + get_int_attr("x1");
    int y1 = parent_y + get_int_attr("y1");
    int x2 = parent_x + get_int_attr("x2");
    int y2 = parent_y + get_int_attr("y2");
    screen.draw_line(x1, y1, x2, y2, true);
  }
  else if (element->get_name() == "circle")
  {
    int x = parent_x + get_int_attr("cx");
    int y = parent_y + get_int_attr("cy");
    int r = get_int_attr("r");
    bool fill = get_bool_attr("fill");

    draw_circle(screen, x, y, r, fill);
  }
  else if (element->get_name() == "bitmap")
  {
    int x = parent_x + get_int_attr("x");
    int y = parent_y + get_int_attr("y");
    std::string src = get_attr("src");

    if (m_bitmaps.count(src))
    {
      screen.draw_bitmap(m_bitmaps.at(src), x, y);
    }
  }

  // Render children (propagate offsets)
  for (auto &child : element->get_children())
  {
    render_element(screen, child, parent_x, parent_y);
  }
}

} // namespace screen_renderer
