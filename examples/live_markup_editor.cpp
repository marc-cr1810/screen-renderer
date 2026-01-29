#include "screen.hpp"
#include "screen_renderer.hpp"
#include "markup_renderer.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <vector>
#include <string>
#include "TextEditor.h"

using namespace screen_renderer;

using namespace screen_renderer;

const int APP_WIDTH = 128;
const int APP_HEIGHT = 64;

// Initial Layout
const char *initial_layout = R"(<screen>
    <rect x="0" y="0" w="128" h="64" fill="false" />
    <line x1="0" y1="10" x2="127" y2="10" />
    <text x="5" y="2" text="LIVE EDITOR" />
    
    <text id="counter" x="10" y="20" text="0" scale="2" />
    <circle cx="100" cy="30" r="10" fill="false" />
</screen>)";

// Suggestion Database
struct TagInfo
{
  std::string description;
  std::vector<std::string> attributes;
};

std::map<std::string, TagInfo> tag_db = {{"screen", {"Root element", {}}},
                                         {"rect", {"Draws a rectangle", {"x", "y", "w", "h", "fill"}}},
                                         {"line", {"Draws a line", {"x1", "y1", "x2", "y2"}}},
                                         {"circle", {"Draws a circle", {"cx", "cy", "r", "fill"}}},
                                         {"text", {"Draws text", {"x", "y", "text", "id", "scale"}}}};

struct SuggestionContext
{
  enum Type
  {
    None,
    Tag,
    Attribute
  };
  Type type = None;
  std::string tag_name;
  std::string partial_input;
};

auto get_suggestion_context(const std::string &line, int column) -> SuggestionContext
{
  if (column < 0 || column > (int)line.length())
    return {};

  // Look backwards for '<'
  int start_tag = -1;
  for (int i = column - 1; i >= 0; --i)
  {
    if (line[i] == '>')
      return {}; // Closed tag
    if (line[i] == '<')
    {
      start_tag = i;
      break;
    }
  }

  if (start_tag == -1)
    return {};

  std::string content = line.substr(start_tag + 1, column - (start_tag + 1));

  // Check if we have spaces
  size_t first_space = content.find(' ');
  if (first_space == std::string::npos)
  {
    // We are typing the tag name
    return {SuggestionContext::Tag, "", content};
  }
  else
  {
    // We are typing attributes
    std::string tag_name = content.substr(0, first_space);

    // Find the last word being typed
    size_t last_space = content.find_last_of(' ');
    std::string partial_attr = content.substr(last_space + 1);

    return {SuggestionContext::Attribute, tag_name, partial_attr};
  }
}

auto main() -> int
{
  if (!glfwInit())
    return -1;
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
    return -1;

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
  MarkupRenderer markup_renderer;

  // Setup Text Editor
  TextEditor editor;
  editor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus()); // Use C++ highlighting for now as XML might not be built-in or similar enough
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
        std::vector<ParseError> errors;
        markup_renderer.load_layout_from_string(current_text, errors);

        TextEditor::ErrorMarkers markers;
        for (const auto &err : errors)
        {
          markers[err.line] = err.message;
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

      if (context.type == SuggestionContext::Tag)
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
      else if (context.type == SuggestionContext::Attribute)
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
      markup_renderer.render(screen);

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
        if (clicked_element && clicked_element->start_line > 0)
        {
          editor.SetCursorPosition({clicked_element->start_line - 1, 0});
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
        int w = 0, h = 0;

        if (selected_element->name == "rect")
        {
          w = selected_element->get_int_attribute("w", 0);
          h = selected_element->get_int_attribute("h", 0);
        }
        else if (selected_element->name == "circle")
        {
          int cx = selected_element->get_int_attribute("cx", 0);
          int cy = selected_element->get_int_attribute("cy", 0);
          int r = selected_element->get_int_attribute("r", 0);
          x = cx - r;
          y = cy - r;
          w = r * 2;
          h = r * 2;
        }
        else if (selected_element->name == "line")
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
        else if (selected_element->name == "text")
        {
          w = selected_element->text_content.length() * 6 * selected_element->get_int_attribute("scale", 1);
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

    // Property Inspector
    {
      ImGui::Begin("Inspector");

      int cursor_line = editor.GetCursorPosition().mLine + 1;
      auto selected_element = markup_renderer.get_element_at_line(cursor_line);

      if (selected_element)
      {
        ImGui::Text("Type: %s", selected_element->name.c_str());
        ImGui::Separator();

        std::vector<std::string> known_int_attrs = {"x", "y", "w", "h", "x1", "y1", "x2", "y2", "cx", "cy", "r", "scale"};
        std::vector<std::string> known_bool_attrs = {"fill"};
        std::vector<std::string> known_string_attrs = {"text", "id"};

        bool changed = false;

        // Copy attributes to a temp map to iterate safely while modifying
        std::map<std::string, std::string> current_attrs = selected_element->attributes;

        if (tag_db.count(selected_element->name))
        {
          for (const auto &attr : tag_db[selected_element->name].attributes)
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
                selected_element->attributes[attr] = std::to_string(v);
                changed = true;
              }
            }
            else if (is_bool)
            {
              bool v = (val == "true");
              if (ImGui::Checkbox(attr.c_str(), &v))
              {
                selected_element->attributes[attr] = v ? "true" : "false";
                changed = true;
              }
            }
            else
            {
              char buffer[256];
              strncpy(buffer, val.c_str(), sizeof(buffer));
              buffer[sizeof(buffer) - 1] = 0;
              if (ImGui::InputText(attr.c_str(), buffer, sizeof(buffer)))
              {
                selected_element->attributes[attr] = buffer;
                changed = true;
              }
            }
            current_attrs.erase(attr);
          }
        }

        for (auto &pair : current_attrs)
        {
          char buffer[256];
          strncpy(buffer, pair.second.c_str(), sizeof(buffer));
          if (ImGui::InputText(pair.first.c_str(), buffer, sizeof(buffer)))
          {
            selected_element->attributes[pair.first] = buffer;
            changed = true;
          }
        }

        if (changed)
        {
          // Reconstruct the XML line
          std::string line = editor.GetTextLines()[selected_element->start_line - 1];

          std::string new_tag = "<" + selected_element->name;
          for (const auto &pair : selected_element->attributes)
          {
            new_tag += " " + pair.first + "=\"" + pair.second + "\"";
          }

          if (line.find("/>") != std::string::npos)
          {
            new_tag += " />";
          }
          else
          {
            new_tag += ">";
          }

          size_t indent = line.find_first_not_of(" \t");
          if (indent != std::string::npos)
          {
            new_tag = line.substr(0, indent) + new_tag;
          }

          if (selected_element->name == "text" && selected_element->start_line == selected_element->end_line)
          {
            auto pos = line.find('>');
            if (pos != std::string::npos)
            {
              new_tag += line.substr(pos + 1);
            }
          }

          auto lines = editor.GetTextLines();
          lines[selected_element->start_line - 1] = new_tag;
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
