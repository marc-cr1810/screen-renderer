#include "bitmap.hpp"
#include "markup_renderer.hpp"
#include "screen.hpp"
#include "screen_renderer.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace screen_renderer;

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
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  // Create window
  GLFWwindow *window = glfwCreateWindow(800, 400, "Styleable Screen Demo", nullptr, nullptr);
  if (!window)
  {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwSetWindowAspectRatio(window, 2, 1);
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, [](GLFWwindow *win, int width, int height) { glViewport(0, 0, width, height); });

  // Initialize GLEW
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK)
  {
    std::cerr << "Failed to initialize GLEW" << std::endl;
    return -1;
  }

  // Create screen and renderer
  screen_t screen(128, 64);
  screen_renderer_t renderer(0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f); // default blue style

  // Load Markup
  markup_renderer_t markup_renderer;
  // Register bitmaps
  markup_renderer.register_bitmap("icon_drone", bitmap_t::create_drone());
  markup_renderer.register_bitmap("icon_battery", bitmap_t::create_battery());
  markup_renderer.register_bitmap("icon_arrow_up", bitmap_t::create_arrow_up());

  // Load Markup (after registering bitmaps)
  if (!markup_renderer.load_layout("examples/assets/sensor_layout.xml"))
  {
    std::cerr << "Failed to load layout file." << std::endl;
    // Continue anyway to show empty screen or crash
  }

  double last_time = glfwGetTime();
  float animation_time = 0.0f;

  while (!glfwWindowShouldClose(window))
  {
    double current_time = glfwGetTime();
    float delta_time = static_cast<float>(current_time - last_time);
    last_time = current_time;
    animation_time += delta_time;

    // Simulate Data Updates
    int detected_drones = static_cast<int>(std::abs(std::sin(animation_time * 0.2f)) * 12);
    int battery = 100 - (int)(animation_time);
    if (battery < 0)
    {
      battery = 0;
    }

    // Update Markup
    markup_renderer.set_text("drone_count", std::to_string(detected_drones));
    markup_renderer.set_text("battery", std::to_string(battery) + "%");
    markup_renderer.set_visible("network", (static_cast<int>(animation_time) % 2) == 0);

    // Simulate GPS updates
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4) << -33.8688 + std::sin(animation_time * 0.1) * 0.0001 << "," << 151.2093 + std::cos(animation_time * 0.1) * 0.0001;
    markup_renderer.set_text("coords", oss.str());

    screen.clear();

    // Render Layout
    markup_renderer.render(screen);

    // Manual Drawing on top of layout

    // 1. CPU Bar Fill (88, 44, 36, 6)
    // val 45-55, max 90
    float cpu_temp = 45.0f + std::sin(animation_time * 0.3f) * 10.0f;
    float max_temp = 90.0f;
    int bar_w = 36;
    int fill_w = (int)((cpu_temp / max_temp) * (float)(bar_w - 2));
    if (fill_w > bar_w - 2)
    {
      fill_w = bar_w - 2;
    }
    if (fill_w < 0)
    {
      fill_w = 0;
    }

    // Draw filled rect at 89, 45 (inside the 88,44 rect with 1px border)
    for (int i = 0; i < fill_w; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        screen.set_pixel(89 + i, 45 + j, true);
      }
    }

    // 2. Activity Graph (68, 20, 56, 20)
    // We need to simulate history since we don't have the struct
    static std::vector<float> history;
    if (history.size() < 40)
    {
      history.push_back(0); // init
    }
    // Update history occasionally
    static float last_update = 0;
    if (animation_time - last_update > 0.5f)
    {
      history.push_back((float)detected_drones);
      if (history.size() > 40)
      {
        history.erase(history.begin());
      }
      last_update = animation_time;
    }

    int graph_x = 68;
    int graph_y = 22;
    int graph_w = 56;
    int graph_h = 20;

    if (!history.empty())
    {
      int points = history.size();
      float x_step = (float)(graph_w - 2) / (float)(points > 1 ? points - 1 : 1);
      int prev_px = -1;
      int prev_py = -1;

      for (int i = 0; i < points; ++i)
      {
        float val = history[i];
        // norm 0-15
        float norm = 1.0f - (val / 15.0f);
        if (norm < 0)
        {
          norm = 0;
        }
        if (norm > 1)
        {
          norm = 1;
        }

        int py = graph_y + 1 + (int)(norm * (graph_h - 3));
        int px = graph_x + 1 + (int)(i * x_step);

        if (px >= graph_x + graph_w - 1)
        {
          px = graph_x + graph_w - 2;
        }

        if (i > 0)
        {
          screen.draw_line(prev_px, prev_py, px, py, true);
        }
        prev_px = px;
        prev_py = py;
      }
    }

    // Render to Window
    glClear(GL_COLOR_BUFFER_BIT);
    renderer.render(screen);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
