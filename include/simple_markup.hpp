#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace screen_renderer
{

struct Element
{
  std::string name;
  std::map<std::string, std::string> attributes;
  std::string text_content;

  std::vector<std::shared_ptr<Element>> children;

  // Source location
  int start_line = 0;
  int end_line = 0;

  // Helper to get attribute with default value
  auto get_attribute(const std::string &key, const std::string &default_value = "") const -> std::string;
  auto get_int_attribute(const std::string &key, int default_value = 0) const -> int;
  auto get_float_attribute(const std::string &key, float default_value = 0.0f) const -> float;
  auto get_bool_attribute(const std::string &key, bool default_value = false) const -> bool;
};

struct ParseError
{
  int line;
  int column;
  std::string message;
};

class SimpleMarkupParser
{
public:
  // Now returns the root element, and populates errors if any
  static auto parse(const std::string &xml_content, std::vector<ParseError> &out_errors) -> std::shared_ptr<Element>;
  static auto parse(const std::string &xml_content) -> std::shared_ptr<Element>; // Legacy overload
  static auto parse_file(const std::string &filename) -> std::shared_ptr<Element>;
};

} // namespace screen_renderer
