#include "simple_markup.hpp"
#include <fstream>
#include <sstream>
#include <stack>
#include <iostream>
#include <algorithm>

namespace screen_renderer
{

auto Element::get_attribute(const std::string &key, const std::string &default_value) const -> std::string
{
  auto it = attributes.find(key);
  if (it != attributes.end())
  {
    return it->second;
  }
  return default_value;
}

auto Element::get_int_attribute(const std::string &key, int default_value) const -> int
{
  auto val = get_attribute(key);
  if (val.empty())
    return default_value;
  try
  {
    return std::stoi(val);
  }
  catch (...)
  {
    return default_value;
  }
}

auto Element::get_float_attribute(const std::string &key, float default_value) const -> float
{
  auto val = get_attribute(key);
  if (val.empty())
    return default_value;
  try
  {
    return std::stof(val);
  }
  catch (...)
  {
    return default_value;
  }
}

auto Element::get_bool_attribute(const std::string &key, bool default_value) const -> bool
{
  auto val = get_attribute(key);
  if (val.empty())
    return default_value;
  return val == "true" || val == "1" || val == "yes";
}

// --- Parser Helper Functions ---

static auto trim(std::string &s) -> void
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
  s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

static auto parse_attributes(const std::string &attr_str) -> std::map<std::string, std::string>
{
  std::map<std::string, std::string> attributes;
  std::string key, value;
  bool in_value = false;

  size_t i = 0;
  while (i < attr_str.length())
  {
    if (!in_value)
    {
      // Find key
      size_t eq_pos = attr_str.find('=', i);
      if (eq_pos == std::string::npos)
        break;

      key = attr_str.substr(i, eq_pos - i);
      trim(key);

      // Find value start quote
      size_t quote_start = attr_str.find('"', eq_pos + 1);
      if (quote_start == std::string::npos)
        break;

      i = quote_start + 1;
      in_value = true;
    }
    else
    {
      // Find value end quote
      size_t quote_end = attr_str.find('"', i);
      if (quote_end == std::string::npos)
        break; // Error

      value = attr_str.substr(i, quote_end - i);
      attributes[key] = value;

      i = quote_end + 1;
      in_value = false;
    }
  }
  return attributes;
}

// Helper to count newlines
static auto count_newlines(const std::string &str, size_t start, size_t end) -> int
{
  int count = 0;
  for (size_t i = start; i < end && i < str.length(); ++i)
  {
    if (str[i] == '\n')
      count++;
  }
  return count;
}

// Find column number (distance from last newline)
static auto get_column(const std::string &str, size_t pos) -> int
{
  if (pos >= str.length())
    pos = str.length();
  size_t last_nl = str.rfind('\n', pos == 0 ? 0 : pos - 1);
  if (last_nl == std::string::npos)
    return static_cast<int>(pos) + 1;
  return static_cast<int>(pos - last_nl);
}

auto SimpleMarkupParser::parse(const std::string &xml_content) -> std::shared_ptr<Element>
{
  std::vector<ParseError> errors;
  return parse(xml_content, errors);
}

auto SimpleMarkupParser::parse(const std::string &xml_content, std::vector<ParseError> &out_errors) -> std::shared_ptr<Element>
{
  std::stack<std::shared_ptr<Element>> element_stack;
  std::stack<std::string> tag_stack; // To track expected closing tags
  std::shared_ptr<Element> root = nullptr;

  size_t pos = 0;
  int current_line = 1;

  while (pos < xml_content.length())
  {
    size_t lt_pos = xml_content.find('<', pos);
    if (lt_pos == std::string::npos)
      break;

    // Update line count
    current_line += count_newlines(xml_content, pos, lt_pos);

    // Check for text content before this tag
    if (!element_stack.empty() && lt_pos > pos)
    {
      std::string text = xml_content.substr(pos, lt_pos - pos);
      // Only add if not just whitespace
      std::string trimmed = text;
      trim(trimmed);
      if (!trimmed.empty())
      {
        element_stack.top()->text_content += text;
      }
    }

    size_t gt_pos = xml_content.find('>', lt_pos);
    if (gt_pos == std::string::npos)
    {
      out_errors.push_back({current_line, get_column(xml_content, lt_pos), "Unclosed tag (missing '>')"});
      break;
    }

    // Check for newlines inside the tag (update count)
    current_line += count_newlines(xml_content, lt_pos, gt_pos);

    std::string tag_content = xml_content.substr(lt_pos + 1, gt_pos - lt_pos - 1);

    // Check if it's a closing tag
    if (!tag_content.empty() && tag_content[0] == '/')
    {
      if (!element_stack.empty())
      {
        std::string tag_name = tag_content.substr(1);
        trim(tag_name);

        if (element_stack.top()->name == tag_name)
        {
          element_stack.top()->end_line = current_line;
          element_stack.pop();
        }
        else
        {
          out_errors.push_back({current_line, get_column(xml_content, lt_pos), "Mismatched closing tag: expected </" + element_stack.top()->name + "> but found </" + tag_name + ">"});
          // Try to recover? For now just ignore or pop if matches parent?
          // Simple recovery: ignore this closing tag
        }
      }
      else
      {
        out_errors.push_back({current_line, get_column(xml_content, lt_pos), "Unexpected closing tag: </" + tag_content.substr(1) + ">"});
      }
      pos = gt_pos + 1;
      continue;
    }

    // It's an opening tag
    bool self_closing = (!tag_content.empty() && tag_content.back() == '/');
    if (self_closing)
    {
      tag_content.pop_back(); // Remove /
    }

    // Parse name and attributes
    std::string name;
    std::string attr_str;

    size_t space_pos = tag_content.find(' ');
    if (space_pos != std::string::npos)
    {
      name = tag_content.substr(0, space_pos);
      attr_str = tag_content.substr(space_pos + 1);
    }
    else
    {
      name = tag_content;
    }
    trim(name);

    if (name.empty())
    {
      out_errors.push_back({current_line, get_column(xml_content, lt_pos), "Empty tag name"});
      pos = gt_pos + 1;
      continue;
    }

    auto new_element = std::make_shared<Element>();
    new_element->name = name;
    new_element->start_line = current_line; // Set start line

    if (!attr_str.empty())
    {
      new_element->attributes = parse_attributes(attr_str);
    }

    if (root == nullptr)
    {
      root = new_element;
    }
    else if (!element_stack.empty())
    {
      element_stack.top()->children.push_back(new_element);
    }

    if (!self_closing)
    {
      element_stack.push(new_element);
    }
    else
    {
      new_element->end_line = current_line; // Self-closing ends on same line (or we should track multi-line tags)
    }

    pos = gt_pos + 1;
  }

  if (!element_stack.empty())
  {
    out_errors.push_back({current_line, static_cast<int>(xml_content.length()), "Unclosed tag: <" + element_stack.top()->name + ">"});
  }

  return root;
}

auto SimpleMarkupParser::parse_file(const std::string &filename) -> std::shared_ptr<Element>
{
  std::ifstream t(filename);
  if (!t.is_open())
    return nullptr;

  std::stringstream buffer;
  buffer << t.rdbuf();
  return parse(buffer.str());
}

} // namespace screen_renderer
