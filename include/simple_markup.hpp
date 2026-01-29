#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace screen_renderer
{

struct element_t
{
public:
  // Accessors
  auto get_name() const -> const std::string &;
  auto set_name(const std::string &name) -> void;

  auto get_attributes() const -> const std::map<std::string, std::string> &;
  auto add_attribute(const std::string &key, const std::string &value) -> void;

  auto get_text_content() const -> const std::string &;
  auto set_text_content(const std::string &text) -> void;

  auto get_children() const -> const std::vector<std::shared_ptr<element_t>> &;
  auto add_child(std::shared_ptr<element_t> child) -> void;

  auto get_start_line() const -> int;
  auto set_start_line(int line) -> void;

  auto get_end_line() const -> int;
  auto set_end_line(int line) -> void;

  // Helper to get attribute with default value
  auto get_attribute(const std::string &key, const std::string &default_value = "") const -> std::string;
  auto get_int_attribute(const std::string &key, int default_value = 0) const -> int;
  auto get_float_attribute(const std::string &key, float default_value = 0.0f) const -> float;
  auto get_bool_attribute(const std::string &key, bool default_value = false) const -> bool;

private:
  std::string m_name;
  std::map<std::string, std::string> m_attributes;
  std::string m_text_content;

  std::vector<std::shared_ptr<element_t>> m_children;

  // Source location
  int m_start_line = 0;
  int m_end_line = 0;
};

struct parse_error_t
{
  int m_line;
  int m_column;
  std::string m_message;
};

class simple_markup_parser_t
{
public:
  // Now returns the root element, and populates errors if any
  static auto parse(const std::string &xml_content, std::vector<parse_error_t> &out_errors) -> std::shared_ptr<element_t>;
  static auto parse(const std::string &xml_content) -> std::shared_ptr<element_t>; // Legacy overload
  static auto parse_file(const std::string &filename) -> std::shared_ptr<element_t>;
};

} // namespace screen_renderer
