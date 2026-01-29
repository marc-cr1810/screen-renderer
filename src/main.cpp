#include "screen.hpp"
#include "screen_renderer.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

using namespace screen_renderer;

auto draw_test_pattern(screen_t &screen) -> void
{
  screen.clear();

  size_t width = screen.get_width();
  size_t height = screen.get_height();

  // Draw a border
  for (size_t x = 0; x < width; ++x)
  {
    screen.set_pixel(x, 0, true);
    screen.set_pixel(x, height - 1, true);
  }

  for (size_t y = 0; y < height; ++y)
  {
    screen.set_pixel(0, y, true);
    screen.set_pixel(width - 1, y, true);
  }

  // Draw diagonal lines
  for (size_t i = 0; i < std::min(width, height); ++i)
  {
    screen.set_pixel(i, i, true);
    if (i < width && (height - 1 - i) < height)
    {
      screen.set_pixel(i, height - 1 - i, true);
    }
  }

  // Draw a checkerboard pattern in the center
  size_t start_x = width / 4;
  size_t start_y = height / 4;
  size_t end_x = 3 * width / 4;
  size_t end_y = 3 * height / 4;
  size_t checker_size = 4;

  for (size_t y = start_y; y < end_y; ++y)
  {
    for (size_t x = start_x; x < end_x; ++x)
    {
      if (((x / checker_size) + (y / checker_size)) % 2 == 0)
      {
        screen.set_pixel(x, y, true);
      }
    }
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
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  // Create window
  GLFWwindow *window = glfwCreateWindow(800, 400, "Screen Renderer", nullptr, nullptr);
  if (!window)
  {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  // Set aspect ratio to match screen dimensions (128:64 = 2:1)
  // This must be called before making the context current for some window managers
  glfwSetWindowAspectRatio(window, 2, 1);

  glfwMakeContextCurrent(window);

  // Set framebuffer resize callback
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

  // Draw test pattern
  draw_test_pattern(screen);

  std::cout << "Screen Renderer started" << std::endl;
  std::cout << "Screen size: " << screen.get_width() << "x" << screen.get_height() << std::endl;

  // Main rendering loop
  while (!glfwWindowShouldClose(window))
  {
    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT);

    // Render screen
    renderer.render(screen);

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup
  glfwTerminate();

  return 0;
}
