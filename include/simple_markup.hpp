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

  // Helper to get attribute with default value
  auto get_attribute(const std::string &key, const std::string &default_value = "") const -> std::string;
  auto get_int_attribute(const std::string &key, int default_value = 0) const -> int;
  auto get_float_attribute(const std::string &key, float default_value = 0.0f) const -> float;
  auto get_bool_attribute(const std::string &key, bool default_value = false) const -> bool;
};

class SimpleMarkupParser
{
public:
  static auto parse(const std::string &xml_content) -> std::shared_ptr<Element>;
  static auto parse_file(const std::string &filename) -> std::shared_ptr<Element>;
};

} // namespace screen_renderer
