#pragma once

#include "bitmap.hpp"
#include "font.hpp"
#include "screen.hpp"
#include "simple_markup.hpp"
#include <map>
#include <string>

namespace screen_renderer
{

class markup_renderer_t
{
public:
  markup_renderer_t();

  // Load layout from XML file
  auto load_layout(const std::string &filename) -> bool;

  // Load layout from XML string
  auto load_layout_from_string(const std::string &xml_content) -> bool;
  // Load layout with error reporting
  auto load_layout_from_string(const std::string &xml_content, std::vector<parse_error_t> &out_errors) -> bool;

  // Render current layout to screen
  auto render(screen_t &screen, float dt = 0.0f) -> void;

  // Root access
  auto get_root() const -> std::shared_ptr<element_t>
  {
    return m_root;
  }

  // Find element by source line
  auto get_element_at_line(int line) -> std::shared_ptr<element_t>;

  // Find element by screen position
  auto get_element_at_pos(int x, int y) -> std::shared_ptr<element_t>;

  // Update text content of an element by ID
  auto set_text(const std::string &id, const std::string &text) -> void;
  // Update visibility of an element by ID
  auto set_visible(const std::string &id, bool visible) -> void;

  // Data binding
  auto set_data(const std::string &key, const std::string &value) -> void;
  auto get_data(const std::string &key, const std::string &default_value = "") const -> std::string;
  auto set_data_list(const std::string &key, const std::vector<std::string> &values) -> void;
  auto clear_data() -> void;

  // Register a bitmap for use in markup
  auto register_bitmap(const std::string &id, const bitmap_t &bitmap) -> void;
  auto get_bitmaps() const -> const std::map<std::string, bitmap_t> &
  {
    return m_bitmaps;
  }

private:
  // Recursive render function with parent offset and parent size (for percentages)
  auto render_element(screen_t &screen, const std::shared_ptr<element_t> &element, int parent_x = 0, int parent_y = 0, int parent_w = 128, int parent_h = 64) -> void;

  // Expression evaluation
  auto evaluate_condition(const std::string &expression) -> bool;

  // Helpers
  auto find_element_by_id(const std::string &id) -> std::shared_ptr<element_t>;
  auto build_id_map(std::shared_ptr<element_t> root) -> void;

  std::shared_ptr<element_t> m_root;
  std::map<std::string, std::shared_ptr<element_t>> m_id_map;
  std::map<std::string, bool> m_visibility_map;                 // Overrides element visibility
  std::map<std::string, std::string> m_text_map;                // Overrides element text
  std::map<std::string, std::string> m_data;                    // Data binding variables
  std::map<std::string, std::vector<std::string>> m_data_lists; // List data

  // Advanced features storage
  std::map<std::string, bitmap_t> m_bitmaps;
  std::map<std::string, std::map<std::string, std::string>> m_styles;
  std::map<std::string, std::shared_ptr<element_t>> m_templates;

  font_t m_font;
  float m_total_time = 0.0f;
};

} // namespace screen_renderer
