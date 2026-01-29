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
#include <map>
#include <algorithm>

using namespace screen_renderer;

const int APP_WIDTH = 128;
const int APP_HEIGHT = 64;

// Initial Layout
// Initial Layout
const char *initial_layout = R"(<screen>
  <text x="10" y="10" text="Hello World" />
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

void update_element_in_editor(TextEditor &editor, std::shared_ptr<element_t> element)
{
  auto lines = editor.GetTextLines();
  if (element->get_start_line() <= 0 || element->get_start_line() > (int)lines.size())
    return;

  std::string line = lines[element->get_start_line() - 1];
  std::string new_tag = "<" + element->get_name();
  for (const auto &pair : element->get_attributes())
    new_tag += " " + pair.first + "=\"" + pair.second + "\"";

  if (line.find("/>") != std::string::npos)
    new_tag += " />";
  else
    new_tag += ">";

  size_t indent = line.find_first_not_of(" \t");
  if (indent != std::string::npos)
    new_tag = line.substr(0, indent) + new_tag;

  lines[element->get_start_line() - 1] = new_tag;
  editor.SetTextLines(lines);
}

void render_tree_node(TextEditor &editor, std::shared_ptr<element_t> element)
{
  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
  if (element->get_children().empty())
    flags |= ImGuiTreeNodeFlags_Leaf;

  bool open = ImGui::TreeNodeEx((void *)element.get(), flags, "%s", element->get_name().c_str());
  if (ImGui::IsItemClicked())
  {
    editor.SetCursorPosition(TextEditor::Coordinates(element->get_start_line() - 1, 0));
  }

  if (open)
  {
    for (auto &child : element->get_children())
    {
      render_tree_node(editor, child);
    }
    ImGui::TreePop();
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
  markup_renderer.register_bitmap("icon_battery", bitmap_t::create_battery());
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

  // Window visibility flags
  static bool show_xml_editor = true;
  static bool show_context_help = true;
  static bool show_screen_preview = true;
  static bool show_structure = false;
  static bool show_mock_data = false;
  static bool show_assets = false;
  static bool show_snippets = false;
  static bool show_inspector = true;
  static bool show_bitmap_editor = false;

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
    animation_time += ImGui::GetIO().DeltaTime;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    if (ImGui::BeginMainMenuBar())
    {
      if (ImGui::BeginMenu("File"))
      {
        if (ImGui::MenuItem("Save", "Ctrl+S"))
        {
          // TODO: Implement save
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Alt+F4"))
        {
          glfwSetWindowShouldClose(window, true);
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Examples"))
      {
        auto load_example = [&](const char *xml)
        {
          editor.SetText(xml);
          std::vector<parse_error_t> errors;
          markup_renderer.load_layout_from_string(xml, errors);

          // Set up mock data for examples
          markup_renderer.set_data("battery", "85%");
          markup_renderer.set_data("drone_count", "12");
          markup_renderer.set_data("status_ok", "true");
          markup_renderer.set_data_list("sensors", {"name:TEMP|value:45", "name:HUMID|value:62", "name:PRESS|value:88"});
        };

        // BASIC EXAMPLES
        if (ImGui::MenuItem("1. Minimal Starter"))
        {
          load_example("<screen>\n  <text x=\"10\" y=\"10\" text=\"Hello World\" />\n</screen>");
        }
        if (ImGui::MenuItem("2. Shapes Gallery"))
        {
          load_example("<screen>\n  <text x=\"2\" y=\"2\" text=\"SHAPES:\" />\n  <rect x=\"10\" y=\"12\" w=\"20\" h=\"15\" fill=\"false\" />\n  <rect x=\"35\" y=\"12\" w=\"20\" h=\"15\" fill=\"true\" />\n  <circle cx=\"72\" cy=\"19\" "
                       "r=\"8\" fill=\"false\" />\n  <circle cx=\"97\" cy=\"19\" r=\"8\" fill=\"true\" />\n  <line x1=\"10\" y1=\"35\" x2=\"118\" y2=\"35\" />\n  <line x1=\"10\" y1=\"40\" x2=\"118\" y2=\"50\" />\n</screen>");
        }

        ImGui::Separator();
        if (ImGui::MenuItem("3. Alignment Demo"))
        {
          load_example(
              "<screen>\n  <text align=\"center\" y=\"2\" text=\"ALIGN DEMO\" />\n  <line x1=\"0\" y1=\"10\" x2=\"128\" y2=\"10\" />\n  \n  <text x=\"0\" y=\"14\" text=\"Left\" align=\"left\" />\n  <text x=\"64\" y=\"14\" "
              "text=\"Center\" align=\"center\" />\n  <text x=\"128\" y=\"14\" text=\"Right\" align=\"right\" />\n  \n  <rect y=\"25\" w=\"30\" h=\"8\" align=\"left\" fill=\"true\" />\n  <rect y=\"25\" w=\"30\" h=\"8\" align=\"center\" "
              "fill=\"false\" />\n  "
              "<rect y=\"25\" w=\"30\" h=\"8\" align=\"right\" fill=\"true\" />\n  \n  <bitmap src=\"icon_drone\" align=\"center\" y=\"38\" />\n  \n  <progress align=\"center\" y=\"52\" w=\"80\" h=\"6\" value=\"70\" />\n</screen>");
        }
        if (ImGui::MenuItem("4. Digital Watch"))
        {
          load_example("<screen>\n  <rect x=\"0\" y=\"0\" w=\"100%\" h=\"100%\" fill=\"false\" />\n  <text align=\"center\" y=\"10\" text=\"CASIO\" scale=\"1\" />\n  <text align=\"center\" y=\"24\" text=\"12:34\" scale=\"3\" />\n  <text "
                       "align=\"center\" y=\"48\" "
                       "text=\"MON 29 JAN\" />\n</screen>");
        }
        if (ImGui::MenuItem("5. Medical Monitor"))
        {
          load_example(
              "<screen>\n  <rect x=\"0\" y=\"0\" w=\"128\" h=\"10\" fill=\"true\" />\n  <text align=\"center\" y=\"2\" text=\"PATIENT MONITOR\" invert=\"true\" />\n  \n  <text x=\"5\" y=\"16\" text=\"HR: 72 BPM\" />\n  <text x=\"70\" "
              "y=\"16\" text=\"SPO2: 98%\" />\n  <text x=\"5\" y=\"26\" text=\"TEMP: 36.5C\" />\n  <text x=\"70\" y=\"26\" text=\"BP: 120/80\" />\n  <text align=\"center\" y=\"40\" text=\"STATUS: STABLE\" />\n</screen>");
        }
        if (ImGui::MenuItem("6. Music Player"))
        {
          load_example("<screen>\n  <text x=\"64\" y=\"8\" text=\"NOW PLAYING\" scale=\"1\" align=\"center\" />\n  <text x=\"64\" y=\"24\" text=\"Neon Dreams\" scale=\"1\" align=\"center\" "
                       "/>\n  <progress x=\"10\" y=\"36\" w=\"108\" h=\"4\" value=\"45\" max=\"100\" />\n  <text x=\"64\" y=\"42\" text=\"01:32 / 03:25\" align=\"center\" />\n  <circle cx=\"34\" cy=\"54\" r=\"4\" fill=\"true\" />\n  "
                       "<circle cx=\"64\" "
                       "cy=\"54\" r=\"6\" fill=\"true\" />\n  <circle cx=\"94\" cy=\"54\" r=\"4\" fill=\"true\" />\n</screen>");
        }
        if (ImGui::MenuItem("6. Retro Game HUD"))
        {
          load_example("<screen>\n  <rect x=\"0\" y=\"0\" w=\"100%\" h=\"12\" fill=\"true\" />\n  <text x=\"4\" y=\"2\" text=\"SCORE:9999\" scale=\"1\" invert=\"true\" />\n  <text x=\"124\" y=\"2\" text=\"LIVES:3\" "
                       "scale=\"1\" invert=\"true\" align=\"right\" />\n  <group x=\"5\" y=\"20\">\n    <bitmap src=\"icon_drone\" x=\"0\" y=\"0\" />\n    <bitmap src=\"icon_drone\" x=\"15\" y=\"0\" />\n    <bitmap src=\"icon_drone\" "
                       "x=\"30\" y=\"0\" "
                       "/>\n  </group>\n  "
                       "<text x=\"64\" y=\"50\" text=\"GET READY!\" pulse=\"2\" scale=\"2\" align=\"center\" />\n</screen>");
        }
        if (ImGui::MenuItem("7. Industrial Control"))
        {
          load_example("<screen>\n  <rect x=\"0\" y=\"0\" w=\"128\" h=\"12\" fill=\"true\" />\n  <text x=\"15\" y=\"2\" text=\"REACTOR STATUS\" invert=\"true\" />\n  <text x=\"4\" y=\"16\" text=\"TEMP:\" />\n  <progress x=\"50\" y=\"16\" "
                       "w=\"70\" h=\"6\" "
                       "value=\"76\" max=\"100\" />\n  <text x=\"4\" y=\"26\" text=\"PRESSURE:\" />\n  <progress x=\"50\" "
                       "y=\"26\" w=\"70\" h=\"6\" value=\"82\" max=\"100\" />\n  <text x=\"4\" y=\"36\" text=\"COOLANT:\" />\n  <progress x=\"50\" y=\"36\" w=\"70\" h=\"6\" value=\"65\" max=\"100\" />\n  <text x=\"40\" y=\"50\" "
                       "text=\"NOMINAL\" />\n</screen>");
        }

        // ADVANCED FEATURES
        ImGui::Separator();
        if (ImGui::MenuItem("8. Weather Station"))
        {
          load_example("<screen>\n  <text x=\"64\" y=\"4\" text=\"WEATHER\" scale=\"1\" align=\"center\" />\n  <circle cx=\"64\" cy=\"24\" r=\"12\" fill=\"false\" />\n  <text x=\"64\" y=\"18\" text=\"23°C\" scale=\"2\" align=\"center\" "
                       "/>\n  <text x=\"64\" y=\"42\" "
                       "text=\"Humidity: 65%\" align=\"center\" />\n  <text x=\"64\" y=\"52\" text=\"Wind: 12 km/h\" align=\"center\" />\n</screen>");
        }
        if (ImGui::MenuItem("9. Data Dashboard"))
        {
          load_example(
              "<screen>\n  <rect x=\"0\" y=\"0\" w=\"128\" h=\"10\" fill=\"true\" />\n  <text x=\"40\" y=\"2\" text=\"ANALYTICS\" invert=\"true\" />\n  <text x=\"4\" y=\"16\" text=\"CPU:\" />\n  <text x=\"35\" y=\"16\" text=\"42%\" />\n  "
              "<progress "
              "x=\"55\" y=\"16\" w=\"68\" h=\"6\" value=\"42\" max=\"100\" />\n  <text x=\"4\" y=\"28\" text=\"RAM:\" />\n  <text x=\"35\" y=\"28\" text=\"68%\" />\n  <progress x=\"55\" y=\"28\" w=\"68\" h=\"6\" value=\"68\" "
              "max=\"100\" />\n  <text x=\"4\" y=\"40\" text=\"DISK:\" />\n  <text x=\"35\" y=\"40\" text=\"55%\" />\n  <progress x=\"55\" y=\"40\" w=\"68\" h=\"6\" value=\"55\" max=\"100\" />\n</screen>");
        }
        if (ImGui::MenuItem("10. Drone HUD (Advanced)"))
        {
          load_example("<screen width=\"128\" height=\"64\">\n  <rect x=\"0\" y=\"0\" w=\"100%\" h=\"12\" fill=\"true\" />\n  <text x=\"4\" y=\"2\" text=\"DRONE-HUD v2.0\" invert=\"true\" />\n  <group x=\"90\" y=\"2\">\n    <bitmap "
                       "src=\"icon_battery\" "
                       "x=\"0\" y=\"0\" />\n    <text x=\"15\" y=\"0\" text=\"{battery}\" invert=\"true\" />\n  </group>\n  <group x=\"0\" y=\"15\">\n    <circle cx=\"64\" cy=\"20\" r=\"10\" fill=\"false\" />\n    <line x1=\"54\" "
                       "y1=\"20\" x2=\"74\" "
                       "y2=\"20\" />\n    <line x1=\"64\" y1=\"10\" x2=\"64\" y2=\"30\" />\n    <if condition=\"{drone_count} > 5\">\n      <rect x=\"34\" y=\"10\" w=\"60\" h=\"20\" fill=\"false\" />\n      <text x=\"40\" y=\"15\" "
                       "text=\"MULTI-TARGET\" pulse=\"2\" />\n    </if>\n  </group>\n  <text x=\"4\" y=\"45\" text=\"ALT: 152m\" />\n  <text x=\"70\" y=\"45\" text=\"SPD: 45kmh\" />\n</screen>");
        }
        if (ImGui::MenuItem("11. System Monitor"))
        {
          load_example(
              "<screen width=\"128\" height=\"64\">\n  <rect x=\"0\" y=\"0\" w=\"128\" h=\"10\" fill=\"true\" />\n  <text x=\"2\" y=\"1\" text=\"SYSTEM MONITOR\" invert=\"true\" />\n  <text x=\"100\" y=\"1\" text=\"{battery}\" "
              "invert=\"true\" "
              "/>\n  <if condition=\"{battery} < 20\">\n    <text x=\"50\" y=\"1\" text=\"LOW!\" pulse=\"3\" invert=\"true\" />\n  </if>\n  <group x=\"5\" y=\"15\">\n    <text x=\"0\" y=\"0\" text=\"SENSORS:\" />\n    <for each=\"s\" "
              "in=\"sensors\">\n      <group y=\"{item_index} * 10 + 10\">\n        <text x=\"0\" y=\"0\" text=\"{s.name}\" />\n        <progress x=\"40\" y=\"0\" w=\"50\" h=\"6\" value=\"{s.value}\" max=\"100\" />\n        <if "
              "condition=\"{s.value} > 80\">\n          <text x=\"95\" y=\"0\" text=\"!!!\" pulse=\"1\" />\n        </if>\n      </group>\n    </for>\n  </group>\n</screen>");
        }
        if (ImGui::MenuItem("12. Minimalist Clock"))
        {
          load_example("<screen>\n  <circle cx=\"64\" cy=\"32\" r=\"28\" fill=\"false\" />\n  <line x1=\"64\" y1=\"32\" x2=\"64\" y2=\"12\" />\n  <line x1=\"64\" y1=\"32\" x2=\"80\" y2=\"40\" />\n  <circle cx=\"64\" cy=\"32\" r=\"2\" "
                       "fill=\"true\" />\n</screen>");
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("View"))
      {
        ImGui::MenuItem("XML Editor", nullptr, &show_xml_editor);
        ImGui::MenuItem("Context Help", nullptr, &show_context_help);
        ImGui::MenuItem("Screen Preview", nullptr, &show_screen_preview);
        ImGui::MenuItem("Structure", nullptr, &show_structure);
        ImGui::MenuItem("Mock Data", nullptr, &show_mock_data);
        ImGui::MenuItem("Assets", nullptr, &show_assets);
        ImGui::MenuItem("Snippets", nullptr, &show_snippets);
        ImGui::MenuItem("Inspector", nullptr, &show_inspector);
        ImGui::MenuItem("Bitmap Editor", nullptr, &show_bitmap_editor);
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }

    // Editor Window
    if (show_xml_editor)
    {
      ImGui::Begin("XML Editor", &show_xml_editor, ImGuiWindowFlags_HorizontalScrollbar);

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
    if (show_context_help)
    {
      ImGui::Begin("Context Help", &show_context_help);

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
    if (show_screen_preview)
    {
      ImGui::Begin("Screen Preview", &show_screen_preview);

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
          editor.SetCursorPosition(TextEditor::Coordinates(clicked_element->get_start_line() - 1, 0));
        }
      }

      // Handle Dragging
      static std::shared_ptr<element_t> dragging_element = nullptr;
      if (ImGui::IsMouseDown(0) && ImGui::IsItemActive())
      {
        auto cur_line = editor.GetCursorPosition().mLine + 1;
        dragging_element = markup_renderer.get_element_at_line(cur_line);
        if (dragging_element)
        {
          ImVec2 delta = ImGui::GetIO().MouseDelta;
          float scale_x = draw_width / (float)APP_WIDTH;
          float scale_y = draw_height / (float)APP_HEIGHT;

          int dx = (int)(delta.x / scale_x);
          int dy = (int)(delta.y / scale_y);

          if (dx != 0 || dy != 0)
          {
            if (dragging_element->get_name() == "line")
            {
              int x1 = dragging_element->get_int_attribute("x1", 0) + dx;
              int y1 = dragging_element->get_int_attribute("y1", 0) + dy;
              int x2 = dragging_element->get_int_attribute("x2", 0) + dx;
              int y2 = dragging_element->get_int_attribute("y2", 0) + dy;
              dragging_element->add_attribute("x1", std::to_string(x1));
              dragging_element->add_attribute("y1", std::to_string(y1));
              dragging_element->add_attribute("x2", std::to_string(x2));
              dragging_element->add_attribute("y2", std::to_string(y2));
            }
            else if (dragging_element->get_name() == "circle")
            {
              int cx = dragging_element->get_int_attribute("cx", 0) + dx;
              int cy = dragging_element->get_int_attribute("cy", 0) + dy;
              dragging_element->add_attribute("cx", std::to_string(cx));
              dragging_element->add_attribute("cy", std::to_string(cy));
            }
            else
            {
              int x = dragging_element->get_int_attribute("x", 0) + dx;
              int y = dragging_element->get_int_attribute("y", 0) + dy;
              dragging_element->add_attribute("x", std::to_string(x));
              dragging_element->add_attribute("y", std::to_string(y));
            }
            update_element_in_editor(editor, dragging_element);
          }
        }
      }
      else
      {
        dragging_element = nullptr;
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

    // Structure Tree
    if (show_structure)
    {
      ImGui::Begin("Structure", &show_structure);
      if (markup_renderer.get_root())
      {
        render_tree_node(editor, markup_renderer.get_root());
      }
      ImGui::End();
    }

    // Mock Data Panel
    if (show_mock_data)
    {
      ImGui::Begin("Mock Data", &show_mock_data);
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
    if (show_assets)
    {
      ImGui::Begin("Assets", &show_assets);
      if (ImGui::Button("Create New Bitmap", ImVec2(-1, 0)))
      {
        show_bitmap_editor = true;
        ImGui::SetWindowFocus("Bitmap Editor");
      }
      ImGui::Separator();
      auto &bitmaps = markup_renderer.get_bitmaps();
      for (const auto &pair : bitmaps)
      {
        ImGui::PushID(pair.first.c_str());

        // Draw small thumbnail (2x scale)
        int thumb_scale = 2;
        ImVec2 size((float)pair.second.get_width() * thumb_scale, (float)pair.second.get_height() * thumb_scale);
        ImVec2 p = ImGui::GetCursorScreenPos();

        // Interaction area
        ImGui::InvisibleButton("##thumb", ImVec2(ImGui::GetContentRegionAvail().x, size.y + 4));
        if (ImGui::IsItemHovered())
        {
          ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::GetColorU32(ImGuiCol_HeaderHovered));
        }
        if (ImGui::IsItemClicked())
        {
          editor.InsertText("<bitmap src=\"" + pair.first + "\" x=\"0\" y=\"0\" />");
        }

        // Draw the bitmap pixels
        auto draw = ImGui::GetWindowDrawList();
        ImVec2 thumb_p = ImVec2(p.x, p.y + 2); // Small vertical padding
        for (size_t y = 0; y < pair.second.get_height(); ++y)
        {
          for (size_t x = 0; x < pair.second.get_width(); ++x)
          {
            if (pair.second.get_pixel(x, y))
            {
              draw->AddRectFilled(ImVec2(thumb_p.x + x * thumb_scale, thumb_p.y + y * thumb_scale), ImVec2(thumb_p.x + (x + 1) * thumb_scale, thumb_p.y + (y + 1) * thumb_scale), IM_COL32(255, 255, 255, 255));
            }
          }
        }

        // Center name vertically relative to thumbnail
        ImGui::SetCursorScreenPos(ImVec2(p.x + size.x + 8, p.y + (size.y + 4 - ImGui::GetTextLineHeight()) * 0.5f));
        ImGui::Text("%s", pair.first.c_str());

        // Restore cursor for next item
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + size.y + 8));
        ImGui::Dummy(ImVec2(0, 0)); // Grow window boundaries after manual cursor positioning

        ImGui::PopID();
      }
      ImGui::End();
    }

    // Snippets
    if (show_snippets)
    {
      ImGui::Begin("Snippets", &show_snippets);
      struct snippet_t
      {
        std::string name;
        std::string code;
      };
      static std::vector<snippet_t> snippets = {
          {"Status Bar", "<rect x=\"0\" y=\"54\" w=\"128\" h=\"10\" fill=\"true\" />\n<text x=\"5\" y=\"56\" text=\"SYSTEM OK\" />"},
          {"Centered Header", "<text x=\"32\" y=\"2\" scale=\"2\" text=\"DASHBOARD\" />"},
          {"Blinking Warning", "<text x=\"10\" y=\"20\" text=\"WARNING\" pulse=\"2\" />"},
          {"Progress Bar", "<progress x=\"10\" y=\"40\" w=\"100\" h=\"8\" value=\"{battery}\" max=\"100\" />"},
          {"[GALLERY] Full Demo",
           "<screen width=\"128\" height=\"64\">\n    <rect x=\"0\" y=\"0\" w=\"128\" h=\"10\" fill=\"true\" />\n    <text x=\"2\" y=\"1\" text=\"SYSTEM MONITOR\" scale=\"1\" color=\"#000000\" />\n    <text x=\"100\" y=\"1\" "
           "text=\"{battery}\" scale=\"1\" />\n    <if condition=\"{battery} < 20\">\n        <text x=\"50\" y=\"1\" text=\"LOW BATT!\" pulse=\"2\" />\n        <rect x=\"0\" y=\"0\" w=\"128\" h=\"10\" fill=\"true\" color=\"#FF0000\" />\n  "
           "  </if>\n    <group x=\"5\" y=\"15\">\n        <text x=\"0\" y=\"0\" text=\"SENSORS:\" />\n        <for each=\"s\" in=\"sensors\">\n            <group y=\"{item_index} * 10 + 10\">\n                <text x=\"0\" y=\"0\" "
           "text=\"{s.name}\" />\n                <progress x=\"40\" y=\"0\" w=\"50\" h=\"6\" value=\"{s.value}\" max=\"100\" />\n            </group>\n        </for>\n    </group>\n    <group x=\"5\" y=\"45\">\n        <text x=\"0\" "
           "y=\"0\" text=\"HISTORY:\" />\n        <graph x=\"40\" y=\"0\" w=\"80\" h=\"15\" data=\"drone_history\" />\n    </group>\n</screen>"},
          {"[GALLERY] Drone HUD", "<screen width=\"128\" height=\"64\">\n    <rect x=\"0\" y=\"0\" w=\"100%\" h=\"12\" fill=\"true\" color=\"#222222\" />\n    <text x=\"4\" y=\"2\" scale=\"1\" text=\"DRONE-HUD v2.0\" />\n    <group "
                                  "x=\"90\" y=\"2\">\n        <bitmap src=\"icon_battery\" x=\"0\" y=\"0\" />\n        <text x=\"15\" y=\"0\" text=\"{battery}\" />\n    </group>\n    <group x=\"0\" y=\"15\">\n        <circle cx=\"64\" "
                                  "cy=\"20\" r=\"10\" fill=\"false\" />\n        <line x1=\"54\" y1=\"20\" x2=\"74\" y2=\"20\" />\n        <line x1=\"64\" y1=\"10\" x2=\"64\" y2=\"30\" />\n    </group>\n    <group x=\"4\" y=\"45\">\n      "
                                  "  <text x=\"0\" y=\"0\" text=\"ALT:\" />\n        <graph x=\"25\" y=\"0\" w=\"40\" h=\"12\" data=\"alt_history\" />\n    </group>\n</screen>"}};

      for (const auto &s : snippets)
      {
        if (ImGui::Button(s.name.c_str(), ImVec2(-1, 0)))
        {
          editor.InsertText(s.code);
        }
      }
      ImGui::End();
    }

    // Property Inspector
    if (show_inspector)
    {
      ImGui::Begin("Inspector", &show_inspector);

      int cursor_line = editor.GetCursorPosition().mLine + 1;
      auto selected_element = markup_renderer.get_element_at_line(cursor_line);

      if (selected_element)
      {
        ImGui::Text("Type: %s", selected_element->get_name().c_str());
        ImGui::Separator();

        std::vector<std::string> known_int_attrs = {"x", "y", "w", "h", "x1", "y1", "x2", "y2", "cx", "cy", "r", "scale", "spacing"};
        std::vector<std::string> known_bool_attrs = {"fill", "visible", "invert"};

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
              // Default values for boolean attributes
              bool default_value = true; // Most defaults are true
              if (attr == "invert")
                default_value = false;

              // If attribute is not set, use default value
              bool v = val.empty() ? default_value : (val == "true" || val == "1");

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
          update_element_in_editor(editor, selected_element);
        }
      }
      else
      {
        ImGui::TextDisabled("Select an element to edit properties.");
      }
      ImGui::End();
    }

    // Bitmap Editor
    if (show_bitmap_editor)
    {
      ImGui::Begin("Bitmap Editor", &show_bitmap_editor);

      static int edit_w = 8;
      static int edit_h = 8;
      static std::vector<bool> pixels(16 * 16, false); // Max size
      static char bitmap_name[64] = "new_icon";

      ImGui::InputText("Name", bitmap_name, 64);

      if (ImGui::Button("8x8"))
      {
        edit_w = edit_h = 8;
        pixels.assign(16 * 16, false);
      }
      ImGui::SameLine();
      if (ImGui::Button("16x16"))
      {
        edit_w = edit_h = 16;
        pixels.assign(16 * 16, false);
      }
      ImGui::SameLine();
      if (ImGui::Button("Clear"))
      {
        std::fill(pixels.begin(), pixels.end(), false);
      }

      ImGui::Separator();

      float cell_size = 20.0f;
      float start_x = ImGui::GetCursorPosX();

      for (int y = 0; y < edit_h; ++y)
      {
        for (int x = 0; x < edit_w; ++x)
        {
          ImGui::PushID(y * edit_w + x);
          bool p = pixels[y * edit_w + x];
          ImVec4 color = p ? ImVec4(1, 1, 1, 1) : ImVec4(0.2f, 0.2f, 0.2f, 1);

          if (ImGui::ColorButton("##pixel", color, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(cell_size, cell_size)))
          {
            pixels[y * edit_w + x] = !p;
          }
          if (x < edit_w - 1)
            ImGui::SameLine();
          ImGui::PopID();
        }
        ImGui::SetCursorPosX(start_x);
      }

      ImGui::Separator();
      if (ImGui::Button("Register Bitmap", ImVec2(-1, 0)))
      {
        std::vector<bool> actual_pixels(edit_w * edit_h);
        for (int i = 0; i < edit_w * edit_h; ++i)
          actual_pixels[i] = pixels[i];
        markup_renderer.register_bitmap(bitmap_name, bitmap_t(edit_w, edit_h, actual_pixels));
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
