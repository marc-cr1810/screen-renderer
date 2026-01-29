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
        markup_renderer.load_layout_from_string(current_text);
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

      // Show texture
      GLuint tex = renderer.get_texture_id();
      renderer.render(screen); // Update texture

      // Get available size
      ImVec2 size = ImGui::GetContentRegionAvail();
      float aspect = 128.0f / 64.0f;
      if (size.x / size.y > aspect)
        size.x = size.y * aspect;
      else
        size.y = size.x / aspect;

      ImGui::Image((ImTextureID)(uint64_t)tex, size, ImVec2(0, 1), ImVec2(1, 0));

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
