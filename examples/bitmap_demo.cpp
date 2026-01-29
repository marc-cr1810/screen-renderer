#include "screen.hpp"
#include "screen_renderer.hpp"
#include "bitmap.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

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
  GLFWwindow *window = glfwCreateWindow(800, 400, "Bitmap Demo - Screen Renderer", nullptr, nullptr);
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

  // Set initial viewport
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);

  // Create screen and renderer
  screen_t screen(128, 64);
  screen_renderer_t renderer(0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f);

  std::cout << "Bitmap Demo started" << std::endl;
  std::cout << "Screen size: " << screen.get_width() << "x" << screen.get_height() << std::endl;

  // Create all bitmap icons
  bitmap_t smiley = bitmap_t::create_smiley();
  bitmap_t heart = bitmap_t::create_heart();
  bitmap_t arrow_up = bitmap_t::create_arrow_up();
  bitmap_t arrow_down = bitmap_t::create_arrow_down();
  bitmap_t arrow_left = bitmap_t::create_arrow_left();
  bitmap_t arrow_right = bitmap_t::create_arrow_right();
  bitmap_t checkmark = bitmap_t::create_checkmark();
  bitmap_t cross = bitmap_t::create_cross();

  // Main rendering loop
  while (!glfwWindowShouldClose(window))
  {
    // Clear screen
    screen.clear();

    // Draw icons in a grid layout
    // Top row
    screen.draw_bitmap(smiley, 10, 5);
    screen.draw_bitmap(heart, 35, 5);
    screen.draw_bitmap(checkmark, 60, 5);
    screen.draw_bitmap(cross, 85, 5);

    // Middle row - Arrows
    screen.draw_bitmap(arrow_up, 10, 28);
    screen.draw_bitmap(arrow_down, 35, 28);
    screen.draw_bitmap(arrow_left, 60, 28);
    screen.draw_bitmap(arrow_right, 85, 28);

    // Draw a border around the entire screen
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

    // Clear OpenGL buffer and render
    glClear(GL_COLOR_BUFFER_BIT);
    renderer.render(screen);

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup
  glfwTerminate();

  return 0;
}
