#include "markup_renderer.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

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

  if (element->get_name() == "template")
  {
    if (!id.empty())
    {
      m_templates[id] = element;
    }
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

auto markup_renderer_t::set_data(const std::string &key, const std::string &value) -> void
{
  m_data[key] = value;
}

auto markup_renderer_t::set_data_list(const std::string &key, const std::vector<std::string> &values) -> void
{
  m_data_lists[key] = values;
}

auto markup_renderer_t::get_data(const std::string &key, const std::string &default_value) const -> std::string
{
  auto it = m_data.find(key);
  if (it != m_data.end())
  {
    return it->second;
  }
  return default_value;
}

auto markup_renderer_t::clear_data() -> void
{
  m_data.clear();
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

static void draw_text_scaled_with_value(screen_t &screen, const font_t &font, const std::string &text, int x, int y, int scale, bool value)
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
              screen.set_pixel(cursor_x + col * scale + sx, y + row * scale + sy, value);
            }
          }
        }
      }
    }
    cursor_x += (bitmap.get_width() + 1) * scale;
  }
}

static void draw_text_scaled(screen_t &screen, const font_t &font, const std::string &text, int x, int y, int scale)
{
  draw_text_scaled_with_value(screen, font, text, x, y, scale, true);
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

auto markup_renderer_t::render(screen_t &screen, float dt) -> void
{
  if (!m_root)
  {
    return;
  }
  m_total_time += dt;
  m_styles.clear();
  m_templates.clear(); // We rebuild them on render if they are in the tree

  render_element(screen, m_root, 0, 0, screen.get_width(), screen.get_height());
}

auto markup_renderer_t::render_element(screen_t &screen, const std::shared_ptr<element_t> &element, int parent_x, int parent_y, int parent_w, int parent_h) -> void
{
  if (!element)
  {
    return;
  }

  // 1. Templates & Styles Definition
  if (element->get_name() == "style")
  {
    std::string id = element->get_attribute("id");
    if (!id.empty())
    {
      m_styles[id] = element->get_attributes();
    }
    return;
  }

  if (element->get_name() == "template")
  {
    // Templates are handled in pre-pass or during ID map build, but let's ensure they aren't rendered
    return;
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
  // Helper to get attribute values, supporting data binding and styles
  auto get_raw_attr = [&](const std::string &key, const std::string &def = "") -> std::string
  {
    std::string val = element->get_attribute(key);
    if (val.empty())
    {
      std::string style_id = element->get_attribute("style");
      if (!style_id.empty() && m_styles.count(style_id))
      {
        if (m_styles[style_id].count(key))
        {
          val = m_styles[style_id].at(key);
        }
      }
    }
    return val.empty() ? def : val;
  };

  auto resolve_data = [&](std::string val) -> std::string
  {
    size_t start_pos = 0;
    while ((start_pos = val.find('{', start_pos)) != std::string::npos)
    {
      size_t end_pos = val.find('}', start_pos);
      if (end_pos == std::string::npos)
        break;

      std::string key = val.substr(start_pos + 1, end_pos - start_pos - 1);
      std::string replacement = get_data(key, "{" + key + "}");
      val.replace(start_pos, end_pos - start_pos + 1, replacement);
      start_pos += replacement.length();
    }
    return val;
  };

  auto get_attr = [&](const std::string &key, const std::string &def = "") -> std::string { return resolve_data(get_raw_attr(key, def)); };

  auto parse_unit = [&](const std::string &val, int total) -> int
  {
    if (val.empty())
      return 0;
    if (val.back() == '%')
    {
      try
      {
        float p = std::stof(val.substr(0, val.size() - 1)) / 100.0f;
        return (int)(p * total);
      }
      catch (...)
      {
        return 0;
      }
    }
    try
    {
      return std::stoi(val);
    }
    catch (...)
    {
      return 0;
    }
  };

  auto get_int_attr = [&](const std::string &key, int def = 0, int total = 0) -> int
  {
    std::string val = get_attr(key);
    if (val.empty())
      return def;
    return parse_unit(val, total);
  };

  auto get_bool_attr = [&](const std::string &key, bool def = false) -> bool
  {
    std::string val = get_attr(key);
    if (val.empty())
      return def;
    return val == "true" || val == "1" || val == "yes";
  };

  // Pulse/Animation check
  std::string pulse_attr = get_attr("pulse");
  if (!pulse_attr.empty())
  {
    try
    {
      float rate = std::stof(pulse_attr);
      if (std::sin(m_total_time * rate * 6.28f) < 0.0f)
        return;
    }
    catch (...)
    {
    }
  }

  // 4. Render specific types

  // Conditional Rendering
  if (element->get_name() == "if")
  {
    std::string condition = get_attr("condition");
    if (evaluate_condition(condition))
    {
      for (auto &child : element->get_children())
      {
        render_element(screen, child, parent_x, parent_y, parent_w, parent_h);
      }
    }
    return;
  }

  // Loop support
  if (element->get_name() == "for")
  {
    std::string each = get_attr("each");
    std::string in = get_attr("in");

    auto it = m_data_lists.find(in);
    if (!each.empty() && !in.empty() && it != m_data_lists.end())
    {
      const auto &items = it->second;
      auto old_data = m_data;

      for (size_t i = 0; i < items.size(); ++i)
      {
        m_data[each] = items[i];
        m_data[each + "_index"] = std::to_string(i);

        for (auto &child : element->get_children())
        {
          render_element(screen, child, parent_x, parent_y, parent_w, parent_h);
        }
      }
      m_data = old_data;
    }
    return;
  }

  // Template instantiation
  if (element->get_name() == "use")
  {
    std::string template_id = get_attr("template");
    if (m_templates.count(template_id))
    {
      auto tpl = m_templates[template_id];
      // Save data, update with attributes from 'use', render, restore
      auto old_data = m_data;
      for (const auto &attr : element->get_attributes())
      {
        m_data[attr.first] = resolve_data(attr.second);
      }

      int ux = get_int_attr("x", 0, parent_w);
      int uy = get_int_attr("y", 0, parent_h);

      for (auto &child : tpl->get_children())
      {
        render_element(screen, child, parent_x + ux, parent_y + uy, parent_w, parent_h);
      }
      m_data = old_data;
    }
    return;
  }

  // Group / Layouts
  if (element->get_name() == "group" || element->get_name() == "hbox" || element->get_name() == "vbox")
  {
    int x = get_int_attr("x", 0, parent_w);
    int y = get_int_attr("y", 0, parent_h);
    int w = get_int_attr("w", parent_w, parent_w);
    int h = get_int_attr("h", parent_h, parent_h);
    int spacing = get_int_attr("spacing", 0, (element->get_name() == "hbox" ? w : h));

    bool is_hbox = element->get_name() == "hbox";
    bool is_vbox = element->get_name() == "vbox";

    int current_offset = 0;
    for (auto &child : element->get_children())
    {
      if (is_hbox)
      {
        render_element(screen, child, parent_x + x + current_offset, parent_y + y, w, h);
        // We need a way to know child's width to auto-layout.
        // For now, let's assume a default width if not specified, or use some simple heuristic.
        // Better: child might have its own 'w' attribute.
        int child_w = child->get_int_attribute("w", 0);
        if (child_w == 0 && child->get_name() == "text")
        {
          std::string content = resolve_data(child->get_attribute("text", child->get_text_content()));
          child_w = content.length() * 6 * child->get_int_attribute("scale", 1);
        }
        current_offset += child_w + spacing;
      }
      else if (is_vbox)
      {
        render_element(screen, child, parent_x + x, parent_y + y + current_offset, w, h);
        int child_h = child->get_int_attribute("h", 0);
        if (child_h == 0 && child->get_name() == "text")
        {
          child_h = 8 * child->get_int_attribute("scale", 1);
        }
        current_offset += child_h + spacing;
      }
      else // Simple group
      {
        render_element(screen, child, parent_x + x, parent_y + y, w, h);
      }
    }
    return;
  }

  if (element->get_name() == "text")
  {
    int x = parent_x + get_int_attr("x", 0, parent_w);
    int y = parent_y + get_int_attr("y", 0, parent_h);
    int scale = get_int_attr("scale", 1);

    std::string content = get_attr("text");
    if (content.empty())
    {
      content = element->get_text_content();
    }
    content = resolve_data(content);

    // Override content if in map
    if (!id.empty())
    {
      auto it = m_text_map.find(id);
      if (it != m_text_map.end())
      {
        content = it->second;
      }
    }

    bool invert = get_bool_attr("invert");
    bool pixel_value = !invert; // If invert, use false (background); otherwise true (foreground)

    if (scale > 1)
    {
      // For scaled text, we need to draw manually with the correct pixel value
      draw_text_scaled_with_value(screen, m_font, content, x, y, scale, pixel_value);
    }
    else
    {
      screen.draw_text(m_font, content, x, y, 1, pixel_value);
    }
  }
  else if (element->get_name() == "rect")
  {
    int x = parent_x + get_int_attr("x", 0, parent_w);
    int y = parent_y + get_int_attr("y", 0, parent_h);
    int w = get_int_attr("w", 0, parent_w);
    int h = get_int_attr("h", 0, parent_h);
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
    int x1 = parent_x + get_int_attr("x1", 0, parent_w);
    int y1 = parent_y + get_int_attr("y1", 0, parent_h);
    int x2 = parent_x + get_int_attr("x2", 0, parent_w);
    int y2 = parent_y + get_int_attr("y2", 0, parent_h);
    screen.draw_line(x1, y1, x2, y2, true);
  }
  else if (element->get_name() == "circle")
  {
    int x = parent_x + get_int_attr("cx", 0, parent_w);
    int y = parent_y + get_int_attr("cy", 0, parent_h);
    int r = get_int_attr("r", 0, std::min(parent_w, parent_h));
    bool fill = get_bool_attr("fill");

    draw_circle(screen, x, y, r, fill);
  }
  else if (element->get_name() == "bitmap")
  {
    int x = parent_x + get_int_attr("x", 0, parent_w);
    int y = parent_y + get_int_attr("y", 0, parent_h);
    std::string src = get_attr("src");
    bool invert = get_bool_attr("invert");

    if (m_bitmaps.count(src))
    {
      bool pixel_value = !invert;
      screen.draw_bitmap(m_bitmaps.at(src), x, y, pixel_value);
    }
  }
  else if (element->get_name() == "progress")
  {
    int x = parent_x + get_int_attr("x", 0, parent_w);
    int y = parent_y + get_int_attr("y", 0, parent_h);
    int w = get_int_attr("w", 20, parent_w);
    int h = get_int_attr("h", 5, parent_h);
    float value = get_int_attr("value", 0);
    float max = get_int_attr("max", 100);

    if (max <= 0)
      max = 1;
    float percent = std::max(0.0f, std::min(1.0f, value / max));

    screen.draw_rect(x, y, w, h, true); // Border
    if (percent > 0)
    {
      int fill_w = (int)(percent * (w - 2));
      if (fill_w > 0)
      {
        for (int i = 0; i < h - 2; ++i)
        {
          screen.draw_line(x + 1, y + 1 + i, x + fill_w, y + 1 + i, true);
        }
      }
    }
  }
  else if (element->get_name() == "graph")
  {
    int x = parent_x + get_int_attr("x", 0, parent_w);
    int y = parent_y + get_int_attr("y", 0, parent_h);
    int w = get_int_attr("w", 30, parent_w);
    int h = get_int_attr("h", 10, parent_h);
    std::string data_key = get_attr("data");

    auto it = m_data_lists.find(data_key);
    if (!data_key.empty() && it != m_data_lists.end())
    {
      const auto &items = it->second;
      if (!items.empty())
      {
        float max_val = 1.0f;
        std::vector<float> values;
        for (const auto &s : items)
        {
          try
          {
            float v = std::stof(s);
            values.push_back(v);
            if (v > max_val)
              max_val = v;
          }
          catch (...)
          {
          }
        }

        if (!values.empty())
        {
          screen.draw_rect(x, y, w, h, true);
          int n = (int)values.size();
          for (int i = 0; i < n - 1 && i < w - 2; ++i)
          {
            int x1 = x + 1 + i;
            int x2 = x + 2 + i;
            int y1 = y + h - 2 - (int)((values[i] / max_val) * (h - 3));
            int y2 = y + h - 2 - (int)((values[i + 1] / max_val) * (h - 3));
            screen.draw_line(x1, y1, x2, y2, true);
          }
        }
      }
    }
  }

  // Render children (propagate offsets/parents)
  for (auto &child : element->get_children())
  {
    // Don't re-render if we already handled them in a layout container
    // Actually, group/hbox/vbox return early, so this only hits elements that aren't those but have children.
    render_element(screen, child, parent_x, parent_y, parent_w, parent_h);
  }
}

auto markup_renderer_t::evaluate_condition(const std::string &expression) -> bool
{
  if (expression.empty())
    return true;

  // Simple direct boolean check
  if (expression == "true" || expression == "1")
    return true;
  if (expression == "false" || expression == "0")
    return false;

  // Check if it's a data key directly
  std::string data_val = get_data(expression);
  if (!data_val.empty())
  {
    return data_val == "true" || data_val == "1";
  }

  // Basic expression evaluation: "{var} op value"
  // 1. Resolve variables
  std::string resolved = expression;
  size_t start_pos = 0;
  while ((start_pos = resolved.find('{', start_pos)) != std::string::npos)
  {
    size_t end_pos = resolved.find('}', start_pos);
    if (end_pos == std::string::npos)
      break;
    std::string key = resolved.substr(start_pos + 1, end_pos - start_pos - 1);
    std::string val = get_data(key, "0");
    resolved.replace(start_pos, end_pos - start_pos + 1, val);
    start_pos += val.length();
  }

  // 2. Find operator
  std::string ops[] = {"==", "!=", "<=", ">=", "<", ">"};
  for (const auto &op : ops)
  {
    size_t op_pos = resolved.find(op);
    if (op_pos != std::string::npos)
    {
      std::string left = resolved.substr(0, op_pos);
      std::string right = resolved.substr(op_pos + op.length());
      // Trim
      left.erase(0, left.find_first_not_of(" "));
      left.erase(left.find_last_not_of(" ") + 1);
      right.erase(0, right.find_first_not_of(" "));
      right.erase(right.find_last_not_of(" ") + 1);

      try
      {
        float l_val = std::stof(left);
        float r_val = std::stof(right);

        if (op == "==")
          return l_val == r_val;
        if (op == "!=")
          return l_val != r_val;
        if (op == "<=")
          return l_val <= r_val;
        if (op == ">=")
          return l_val >= r_val;
        if (op == "<")
          return l_val < r_val;
        if (op == ">")
          return l_val > r_val;
      }
      catch (...)
      {
        // String comparison fallback
        if (op == "==")
          return left == right;
        if (op == "!=")
          return left != right;
      }
    }
  }

  return false;
}

} // namespace screen_renderer
