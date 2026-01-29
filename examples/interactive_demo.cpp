#include "screen.hpp"
#include "screen_renderer.hpp"
#include "bitmap.hpp"
#include "font.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <cstring>

using namespace screen_renderer;

// Global state
static char text_input[256] = "Hello World!";
static int text_x = 5;
static int text_y = 5;
static int text_spacing = 1;
static int bitmap_type = 0;
static int bitmap_x = 50;
static int bitmap_y = 20;
static float bg_color[3] = {0.0f, 0.1f, 0.2f};
static float pixel_color[3] = {1.0f, 1.0f, 0.0f};
static bool mouse_down = false;

const char *bitmap_names[] = {"Smiley", "Heart", "Arrow Up", "Arrow Down", "Arrow Left", "Arrow Right", "Checkmark", "Cross"};

auto get_selected_bitmap() -> bitmap_t
{
  switch (bitmap_type)
  {
  case 0:
    return bitmap_t::create_smiley();
  case 1:
    return bitmap_t::create_heart();
  case 2:
    return bitmap_t::create_arrow_up();
  case 3:
    return bitmap_t::create_arrow_down();
  case 4:
    return bitmap_t::create_arrow_left();
  case 5:
    return bitmap_t::create_arrow_right();
  case 6:
    return bitmap_t::create_checkmark();
  case 7:
    return bitmap_t::create_cross();
  default:
    return bitmap_t::create_smiley();
  }
}

auto main() -> int
{
  // Initialize GLFW
  if (!glfwInit())
  {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }

  // Configure GLFW
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create window
  GLFWwindow *window = glfwCreateWindow(1400, 700, "Interactive Screen Renderer - Testing Tool", nullptr, nullptr);
  if (!window)
  {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  // Initialize GLEW
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK)
  {
    std::cerr << "Failed to initialize GLEW" << std::endl;
    return -1;
  }

  // Set up ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // Create screen and renderer
  screen_t screen(128, 64);
  screen_renderer_t renderer(bg_color[0], bg_color[1], bg_color[2], pixel_color[0], pixel_color[1], pixel_color[2]);
  font_t font;

  std::cout << "Interactive Screen Renderer started" << std::endl;
  std::cout << "Screen size: " << screen.get_width() << "x" << screen.get_height() << std::endl;

  // Main loop
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Create dockspace
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    // Create ImGui UI
    {
      ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
      ImGui::SetNextWindowSize(ImVec2(400, 680), ImGuiCond_FirstUseEver);
      ImGui::Begin("Screen Renderer Controls");

      if (ImGui::CollapsingHeader("Screen Info", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::Text("Resolution: %zux%zu", screen.get_width(), screen.get_height());
        int pixel_count = 0;
        for (size_t y = 0; y < screen.get_height(); ++y)
        {
          for (size_t x = 0; x < screen.get_width(); ++x)
          {
            if (screen.get_pixel(x, y))
              pixel_count++;
          }
        }
        ImGui::Text("Active Pixels: %d / %zu", pixel_count, screen.get_width() * screen.get_height());
      }

      if (ImGui::CollapsingHeader("Text Rendering", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::InputText("Text", text_input, sizeof(text_input));
        ImGui::SliderInt("Text X", &text_x, 0, 120);
        ImGui::SliderInt("Text Y", &text_y, 0, 57);
        ImGui::SliderInt("Spacing", &text_spacing, 0, 5);

        if (ImGui::Button("Render Text"))
        {
          screen.draw_text(font, text_input, text_x, text_y, text_spacing);
        }
      }

      if (ImGui::CollapsingHeader("Bitmap Tools", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::Combo("Bitmap Type", &bitmap_type, bitmap_names, IM_ARRAYSIZE(bitmap_names));
        ImGui::SliderInt("Bitmap X", &bitmap_x, 0, 120);
        ImGui::SliderInt("Bitmap Y", &bitmap_y, 0, 60);

        if (ImGui::Button("Place Bitmap"))
        {
          bitmap_t bmp = get_selected_bitmap();
          screen.draw_bitmap(bmp, bitmap_x, bitmap_y);
        }
      }

      if (ImGui::CollapsingHeader("Screen Controls", ImGuiTreeNodeFlags_DefaultOpen))
      {
        if (ImGui::Button("Clear Screen"))
        {
          screen.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("Fill Screen"))
        {
          screen.fill();
        }

        if (ImGui::Button("Invert Screen"))
        {
          for (size_t y = 0; y < screen.get_height(); ++y)
          {
            for (size_t x = 0; x < screen.get_width(); ++x)
            {
              screen.set_pixel(x, y, !screen.get_pixel(x, y));
            }
          }
        }

        // Draw border button
        if (ImGui::Button("Draw Border"))
        {
          for (size_t x = 0; x < screen.get_width(); ++x)
          {
            screen.set_pixel(x, 0, true);
            screen.set_pixel(x, screen.get_height() - 1, true);
          }
          for (size_t y = 0; y < screen.get_height(); ++y)
          {
            screen.set_pixel(0, y, true);
            screen.set_pixel(screen.get_width() - 1, y, true);
          }
        }
      }

      if (ImGui::CollapsingHeader("Color Configuration"))
      {
        bool color_changed = false;
        color_changed |= ImGui::ColorEdit3("Background Color", bg_color);
        color_changed |= ImGui::ColorEdit3("Pixel Color", pixel_color);

        if (color_changed)
        {
          renderer.set_background_color(bg_color[0], bg_color[1], bg_color[2]);
          renderer.set_pixel_color(pixel_color[0], pixel_color[1], pixel_color[2]);
        }
      }

      if (ImGui::CollapsingHeader("Pixel Drawing"))
      {
        ImGui::Text("Click on the screen to toggle pixels");
        ImGui::Text("Hold mouse button and drag to draw");
      }

      ImGui::End();
    }

    // Screen Display Window
    {
      ImGui::SetNextWindowPos(ImVec2(420, 10), ImGuiCond_FirstUseEver);
      ImGui::SetNextWindowSize(ImVec2(960, 680), ImGuiCond_FirstUseEver);
      ImGui::Begin("Screen Display");

      // Get the texture ID from the renderer
      GLuint texture_id = renderer.get_texture_id();

      // Calculate the display size maintaining aspect ratio
      ImVec2 window_size = ImGui::GetContentRegionAvail();
      float screen_aspect = static_cast<float>(screen.get_width()) / static_cast<float>(screen.get_height());
      float window_aspect = window_size.x / window_size.y;

      ImVec2 image_size;
      if (window_aspect > screen_aspect)
      {
        // Window is wider than screen, limit by height
        image_size.y = window_size.y;
        image_size.x = window_size.y * screen_aspect;
      }
      else
      {
        // Window is taller than screen, limit by width
        image_size.x = window_size.x;
        image_size.y = window_size.x / screen_aspect;
      }

      // Center the image in the window
      ImVec2 cursor_pos = ImGui::GetCursorPos();
      cursor_pos.x += (window_size.x - image_size.x) * 0.5f;
      cursor_pos.y += (window_size.y - image_size.y) * 0.5f;
      ImGui::SetCursorPos(cursor_pos);

      // Store image position for mouse input
      ImVec2 image_pos = ImGui::GetCursorScreenPos();

      // Display the texture (flip vertically because framebuffer textures are upside down)
      ImVec2 uv0(0.0f, 1.0f); // Top-left becomes bottom-left
      ImVec2 uv1(1.0f, 0.0f); // Bottom-right becomes top-right
      ImGui::Image((ImTextureID)(uint64_t)texture_id, image_size, uv0, uv1);

      // Handle mouse input on the image
      if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
      {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        float relative_x = (mouse_pos.x - image_pos.x) / image_size.x;
        float relative_y = (mouse_pos.y - image_pos.y) / image_size.y;

        if (relative_x >= 0.0f && relative_x <= 1.0f && relative_y >= 0.0f && relative_y <= 1.0f)
        {
          size_t pixel_x = static_cast<size_t>(relative_x * screen.get_width());
          size_t pixel_y = static_cast<size_t>(relative_y * screen.get_height());

          if (pixel_x < screen.get_width() && pixel_y < screen.get_height())
          {
            screen.set_pixel(pixel_x, pixel_y, true);
          }
        }
      }

      ImGui::End();
    }

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Render the screen to its texture
    renderer.render(screen);

    // Render ImGui
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwTerminate();

  return 0;
}
