#include "markup_renderer.hpp"
#include "screen.hpp"
#include "screen_renderer.hpp"
#include "TextEditor.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <string>
#include <vector>

using namespace screen_renderer;

const int APP_WIDTH = 128;
const int APP_HEIGHT = 64;

// Initial Layout
// Initial Layout
const char *initial_layout = R"(<screen>
    <style id="header" scale="1" />
    <style id="big" scale="3" />
    
    <rect x="0" y="0" w="128" h="64" fill="false" />
    <line x1="0" y1="10" x2="127" y2="10" />
    <text style="header" x="5" y="2" text="LIVE PREVIEW" />
    
    <group x="105" y="2">
        <bitmap src="icon_battery" x="-13" y="0" />
        <text style="header" x="0" y="0" text="80%" />
    </group>
    
    <group x="10" y="20">
        <text style="header" x="0" y="0" text="COUNT" />
        <text id="counter" style="big" x="0" y="10" text="0" />
        <bitmap src="icon_drone" x="30" y="20" />
    </group>

    <line x1="0" y1="54" x2="100%" y2="54" />
    <text id="status" x="5" y="56" text="SYSTEM READY" visible="true" />
</screen>)";

// Suggestion Database
struct tag_info_t
{
  std::string description;
  std::vector<std::string> attributes;
};

std::map<std::string, tag_info_t> tag_db = {{"screen", {"Root element", {}}},
                                            {"rect", {"Draws a rectangle", {"x", "y", "w", "h", "fill", "visible", "style", "pulse", "id"}}},
                                            {"line", {"Draws a line", {"x1", "y1", "x2", "y2", "visible", "style", "pulse", "id"}}},
                                            {"circle", {"Draws a circle", {"cx", "cy", "r", "fill", "visible", "style", "pulse", "id"}}},
                                            {"text", {"Draws text", {"x", "y", "text", "id", "scale", "visible", "style", "pulse"}}},
                                            {"group", {"Groups elements", {"x", "y", "visible", "style", "id"}}},
                                            {"hbox", {"Horizontal layout", {"x", "y", "w", "h", "spacing", "visible", "style", "id"}}},
                                            {"vbox", {"Vertical layout", {"x", "y", "w", "h", "spacing", "visible", "style", "id"}}},
                                            {"if", {"Conditional rendering", {"condition"}}},
                                            {"template", {"Defines a template", {"id"}}},
                                            {"use", {"Uses a template", {"template", "x", "y"}}},
                                            {"bitmap", {"Draws a bitmap", {"x", "y", "src", "visible", "style", "id"}}},
                                            {"style", {"Defines a style", {"id", "scale", "fill", "pulse"}}}};

struct suggestion_context_t
{
  enum type_e
  {
    None,
    Tag,
    Attribute
  };
  type_e type = None;
  std::string tag_name;
  std::string partial_input;
};

auto get_suggestion_context(const std::string &line, int column) -> suggestion_context_t
{
  if (column < 0 || column > (int)line.length())
  {
    return {};
  }

  // Look backwards for '<'
  int start_tag = -1;
  for (int i = column - 1; i >= 0; --i)
  {
    if (line[i] == '>')
    {
      return {}; // Closed tag
    }
    if (line[i] == '<')
    {
      start_tag = i;
      break;
    }
  }

  if (start_tag == -1)
  {
    return {};
  }

  std::string content = line.substr(start_tag + 1, column - (start_tag + 1));

  // Check if we have spaces
  size_t first_space = content.find(' ');
  if (first_space == std::string::npos)
  {
    // We are typing the tag name
    return {suggestion_context_t::Tag, "", content};
  }
  else
  {
    // We are typing attributes
    std::string tag_name = content.substr(0, first_space);

    // Find the last word being typed
    size_t last_space = content.find_last_of(' ');
    std::string partial_attr = content.substr(last_space + 1);

    return {suggestion_context_t::Attribute, tag_name, partial_attr};
  }
}

auto main() -> int
{
  if (!glfwInit())
  {
    return -1;
  }
  // ... (existing window setup)

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(1200, 700, "Live Markup Editor", nullptr, nullptr);
  if (!window)
  {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  if (glewInit() != GLEW_OK)
  {
    return -1;
  }

  // ImGui Setup
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  screen_t screen(128, 64);
  screen_renderer_t renderer(0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f); // Default Blue
  markup_renderer_t markup_renderer;
  markup_renderer.register_bitmap("icon_drone", bitmap_t::create_smiley());
  markup_renderer.register_bitmap("icon_battery", bitmap_t::create_heart());
  markup_renderer.register_bitmap("icon_arrow_up", bitmap_t::create_arrow_up());

  // Setup Text Editor
  TextEditor editor;

  TextEditor::LanguageDefinition lang;
  lang.mName = "XML";
  lang.mCommentStart = "<!--";
  lang.mCommentEnd = "-->";

  // Custom tokens for XML
  lang.mTokenRegexStrings.push_back({"<[/]?[a-zA-Z0-9]+", TextEditor::PaletteIndex::Keyword});        // Tag start/end
  lang.mTokenRegexStrings.push_back({"[a-zA-Z0-9_-]+(?=\\=)", TextEditor::PaletteIndex::Identifier}); // Attribute name
  lang.mTokenRegexStrings.push_back({"\"[^\"]*\"", TextEditor::PaletteIndex::String});                // Attribute value
  lang.mTokenRegexStrings.push_back({">", TextEditor::PaletteIndex::Keyword});                        // Tag end
  lang.mTokenRegexStrings.push_back({"/>", TextEditor::PaletteIndex::Keyword});                       // Self-closing tag end

  editor.SetLanguageDefinition(lang);
  // Better yet, check if XML exists? Usually logic/C++/SQL/AngelScript/Lua/Shader.
  // Let's stick to C++ or just set null, or try to define custom later.
  // Actually, standard library usually has C/C++/SQL/Lua. XML might be missing.
  // But C++ colors are okay for tags.
  editor.SetText(initial_layout);

  // Initial load
  markup_renderer.load_layout_from_string(editor.GetText());

  float animation_time = 0.0f;

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
    animation_time += ImGui::GetIO().DeltaTime;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    // Editor Window
    {
      ImGui::Begin("XML Editor", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
      if (ImGui::BeginMenuBar())
      {
        if (ImGui::BeginMenu("File"))
        {
          if (ImGui::MenuItem("Save"))
          {
            // Logic to save to file could go here
          }
          ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
      }

      // Render editor
      editor.Render("TextEditor");

      // Reload on change logic?
      // TextEditor doesn't have an "IsChanged" frame flag easily, but we can check text.
      // For performance, maybe only update every few frames or check text length/hash.
      // Or simply update every frame for this small text.
      static std::string last_text = "";
      std::string current_text = editor.GetText();
      if (current_text != last_text)
      {
        std::vector<parse_error_t> errors;
        markup_renderer.load_layout_from_string(current_text, errors);

        TextEditor::ErrorMarkers markers;
        for (const auto &err : errors)
        {
          markers[err.m_line] = err.m_message;
        }
        editor.SetErrorMarkers(markers);

        last_text = current_text;
      }

      ImGui::End();
    }

    // Suggestions Window
    {
      ImGui::Begin("Context Help");

      auto cursor = editor.GetCursorPosition();
      std::string line = editor.GetCurrentLineText();
      auto context = get_suggestion_context(line, cursor.mColumn);

      if (context.type == suggestion_context_t::Tag)
      {
        ImGui::Text("Suggesting Tags for '<%s':", context.partial_input.c_str());
        ImGui::Separator();
        for (const auto &pair : tag_db)
        {
          if (pair.first.find(context.partial_input) == 0)
          { // Prefix match
            if (ImGui::Selectable(pair.first.c_str()))
            {
              editor.InsertText(pair.first.substr(context.partial_input.length()) + " ");
            }
            if (ImGui::IsItemHovered())
            {
              ImGui::SetTooltip("%s", pair.second.description.c_str());
            }
          }
        }
      }
      else if (context.type == suggestion_context_t::Attribute)
      {
        ImGui::Text("Attributes for <%s> (matching '%s'):", context.tag_name.c_str(), context.partial_input.c_str());
        ImGui::Separator();
        if (tag_db.find(context.tag_name) != tag_db.end())
        {
          const auto &info = tag_db[context.tag_name];
          ImGui::TextWrapped("%s", info.description.c_str());
          ImGui::Separator();
          for (const auto &attr : info.attributes)
          {
            if (attr.find(context.partial_input) == 0)
            {
              if (ImGui::Selectable(attr.c_str()))
              {
                editor.InsertText(attr.substr(context.partial_input.length()) + "=\"\"");
                editor.MoveLeft(1); // Move inside quotes
              }
            }
          }
        }
        else
        {
          ImGui::TextDisabled("Unknown tag: %s", context.tag_name.c_str());
        }
      }
      else
      {
        ImGui::TextDisabled("Type '<' to see tag suggestions.");
        ImGui::TextDisabled("Type inside a tag to see attributes.");
      }

      ImGui::End();
    }

    // Screen View
    {
      ImGui::Begin("Screen Preview");

      // Logical Update
      markup_renderer.set_text("counter", std::to_string((int)animation_time));

      // Draw
      screen.clear();
      markup_renderer.render(screen, ImGui::GetIO().DeltaTime);

      // Draw the screen buffer
      // Maintain aspect ratio
      float aspect = (float)APP_WIDTH / (float)APP_HEIGHT;
      float win_width = ImGui::GetContentRegionAvail().x;
      float win_height = ImGui::GetContentRegionAvail().y;

      float draw_width = win_width;
      float draw_height = win_width / aspect;

      if (draw_height > win_height)
      {
        draw_height = win_height;
        draw_width = draw_height * aspect;
      }

      ImVec2 start_pos = ImGui::GetCursorScreenPos();
      // Update the texture before drawing
      renderer.render(screen);
      GLuint screen_texture = renderer.get_texture_id();
      ImGui::Image((void *)(intptr_t)screen_texture, ImVec2(draw_width, draw_height), ImVec2(0, 1), ImVec2(1, 0));

      // Handle Screen Picking
      if (ImGui::IsItemClicked(0))
      {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImVec2 image_min = ImGui::GetItemRectMin();

        float scale_x = draw_width / (float)APP_WIDTH;
        float scale_y = draw_height / (float)APP_HEIGHT;

        int screen_x = (int)((mouse_pos.x - image_min.x) / scale_x);
        int screen_y = (int)((mouse_pos.y - image_min.y) / scale_y);

        auto clicked_element = markup_renderer.get_element_at_pos(screen_x, screen_y);
        if (clicked_element && clicked_element->get_start_line() > 0)
        {
          editor.SetCursorPosition({clicked_element->get_start_line() - 1, 0});
          // Optional: Highlight selection
          // editor.SetSelection({clicked_element->start_line - 1, 0}, {clicked_element->end_line - 1, 1000});
        }
      }

      // Draw Highlight Overlay
      int cursor_line = editor.GetCursorPosition().mLine + 1;
      auto selected_element = markup_renderer.get_element_at_line(cursor_line);

      if (selected_element)
      {
        int x = selected_element->get_int_attribute("x", 0);
        int y = selected_element->get_int_attribute("y", 0);
        int w = 0;
        int h = 0;

        if (selected_element->get_name() == "rect")
        {
          w = selected_element->get_int_attribute("w", 0);
          h = selected_element->get_int_attribute("h", 0);
        }
        else if (selected_element->get_name() == "circle")
        {
          int cx = selected_element->get_int_attribute("cx", 0);
          int cy = selected_element->get_int_attribute("cy", 0);
          int r = selected_element->get_int_attribute("r", 0);
          x = cx - r;
          y = cy - r;
          w = r * 2;
          h = r * 2;
        }
        else if (selected_element->get_name() == "line")
        {
          int x1 = selected_element->get_int_attribute("x1", 0);
          int y1 = selected_element->get_int_attribute("y1", 0);
          int x2 = selected_element->get_int_attribute("x2", 0);
          int y2 = selected_element->get_int_attribute("y2", 0);
          x = std::min(x1, x2);
          y = std::min(y1, y2);
          w = std::abs(x2 - x1);
          h = std::abs(y2 - y1);
        }
        else if (selected_element->get_name() == "text")
        {
          w = selected_element->get_text_content().length() * 6 * selected_element->get_int_attribute("scale", 1);
          h = 8 * selected_element->get_int_attribute("scale", 1);
        }

        if (w > 0 && h > 0)
        {
          float scale_x = draw_width / (float)APP_WIDTH;
          float scale_y = draw_height / (float)APP_HEIGHT;

          ImVec2 p_min = ImVec2(start_pos.x + x * scale_x, start_pos.y + y * scale_y);
          ImVec2 p_max = ImVec2(p_min.x + w * scale_x, p_min.y + h * scale_y);

          ImGui::GetWindowDrawList()->AddRect(p_min, p_max, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
        }
      }

      ImGui::End();
    }

    // Mock Data Panel
    {
      ImGui::Begin("Mock Data");
      static std::map<std::string, std::string> mock_vars = {{"battery", "80%"}, {"drone_count", "12"}, {"status_ok", "true"}, {"status_warning", "false"}};
      for (auto &pair : mock_vars)
      {
        char val[64];
        strncpy(val, pair.second.c_str(), 64);
        if (ImGui::InputText(pair.first.c_str(), val, 64))
        {
          pair.second = val;
          markup_renderer.set_data(pair.first, val);
        }
      }
      // Initialize if first frame
      static bool first = true;
      if (first)
      {
        for (auto &p : mock_vars)
          markup_renderer.set_data(p.first, p.second);
        first = false;
      }
      ImGui::End();
    }

    // Asset Browser
    {
      ImGui::Begin("Assets");
      std::vector<std::string> assets = {"icon_drone", "icon_battery", "icon_arrow_up"};
      for (const auto &name : assets)
      {
        if (ImGui::Selectable(name.c_str()))
        {
          editor.InsertText("<bitmap src=\"" + name + "\" x=\"0\" y=\"0\" />");
        }
      }
      ImGui::End();
    }

    // Property Inspector
    {
      ImGui::Begin("Inspector");

      int cursor_line = editor.GetCursorPosition().mLine + 1;
      auto selected_element = markup_renderer.get_element_at_line(cursor_line);

      if (selected_element)
      {
        ImGui::Text("Type: %s", selected_element->get_name().c_str());
        ImGui::Separator();

        std::vector<std::string> known_int_attrs = {"x", "y", "w", "h", "x1", "y1", "x2", "y2", "cx", "cy", "r", "scale", "spacing"};
        std::vector<std::string> known_bool_attrs = {"fill", "visible"};

        bool changed = false;
        std::map<std::string, std::string> current_attrs = selected_element->get_attributes();

        if (tag_db.count(selected_element->get_name()))
        {
          for (const auto &attr : tag_db[selected_element->get_name()].attributes)
          {
            std::string val = selected_element->get_attribute(attr);
            bool is_int = std::find(known_int_attrs.begin(), known_int_attrs.end(), attr) != known_int_attrs.end();
            bool is_bool = std::find(known_bool_attrs.begin(), known_bool_attrs.end(), attr) != known_bool_attrs.end();

            if (is_int)
            {
              int v = 0;
              try
              {
                v = std::stoi(val);
              }
              catch (...)
              {
              }
              if (ImGui::DragInt(attr.c_str(), &v))
              {
                selected_element->add_attribute(attr, std::to_string(v));
                changed = true;
              }
            }
            else if (is_bool)
            {
              bool v = (val == "true" || val == "1");
              if (ImGui::Checkbox(attr.c_str(), &v))
              {
                selected_element->add_attribute(attr, v ? "true" : "false");
                changed = true;
              }
            }
            else
            {
              char buffer[256];
              strncpy(buffer, val.c_str(), 256);
              if (ImGui::InputText(attr.c_str(), buffer, 256))
              {
                selected_element->add_attribute(attr, buffer);
                changed = true;
              }
            }
            current_attrs.erase(attr);
          }
        }

        for (auto &pair : current_attrs)
        {
          char buffer[256];
          strncpy(buffer, pair.second.c_str(), 256);
          if (ImGui::InputText(pair.first.c_str(), buffer, 256))
          {
            selected_element->add_attribute(pair.first, buffer);
            changed = true;
          }
        }

        if (changed)
        {
          auto lines = editor.GetTextLines();
          std::string line = lines[selected_element->get_start_line() - 1];
          std::string new_tag = "<" + selected_element->get_name();
          for (const auto &pair : selected_element->get_attributes())
            new_tag += " " + pair.first + "=\"" + pair.second + "\"";

          if (line.find("/>") != std::string::npos)
            new_tag += " />";
          else
            new_tag += ">";

          size_t indent = line.find_first_not_of(" \t");
          if (indent != std::string::npos)
            new_tag = line.substr(0, indent) + new_tag;

          lines[selected_element->get_start_line() - 1] = new_tag;
          editor.SetTextLines(lines);
        }
      }
      else
      {
        ImGui::TextDisabled("Select an element to edit properties.");
      }
      ImGui::End();
    }

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();
  return 0;
}
