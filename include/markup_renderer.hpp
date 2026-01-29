#pragma once

#include "simple_markup.hpp"
#include "screen.hpp"
#include "font.hpp"
#include <map>
#include <string>

namespace screen_renderer
{

class MarkupRenderer
{
public:
  MarkupRenderer();

  // Load layout from XML file
  auto load_layout(const std::string &filename) -> bool;

  // Load layout from XML string
  auto load_layout_from_string(const std::string &xml_content) -> bool;
  // Load layout with error reporting
  auto load_layout_from_string(const std::string &xml_content, std::vector<ParseError> &out_errors) -> bool;

  // Render current layout to screen
  auto render(screen_t &screen) -> void;

  // Find element by source line
  auto get_element_at_line(int line) -> std::shared_ptr<Element>;

  // Find element by screen position
  auto get_element_at_pos(int x, int y) -> std::shared_ptr<Element>;

  // Update text content of an element by ID
  auto set_text(const std::string &id, const std::string &text) -> void;

  // Update visibility of an element by ID
  auto set_visible(const std::string &id, bool visible) -> void;

private:
  // Recursive render function
  auto render_element(screen_t &screen, const std::shared_ptr<Element> &element) -> void;

  // Helpers
  auto find_element_by_id(const std::string &id) -> std::shared_ptr<Element>;
  auto build_id_map(std::shared_ptr<Element> root) -> void;

  std::shared_ptr<Element> m_root;
  std::map<std::string, std::shared_ptr<Element>> m_id_map;
  std::map<std::string, bool> m_visibility_map;  // Overrides element visibility
  std::map<std::string, std::string> m_text_map; // Overrides element text

  font_t m_font;
};

} // namespace screen_renderer
