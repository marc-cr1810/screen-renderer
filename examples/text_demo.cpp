#include "screen.hpp"
#include "screen_renderer.hpp"
#include "font.hpp"
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
  GLFWwindow *window = glfwCreateWindow(800, 400, "Text Demo - Screen Renderer", nullptr, nullptr);
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

  // Create font
  font_t font;

  std::cout << "Text Demo started" << std::endl;
  std::cout << "Screen size: " << screen.get_width() << "x" << screen.get_height() << std::endl;
  std::cout << "Font size: " << font.get_char_width() << "x" << font.get_char_height() << std::endl;

  // Main rendering loop
  while (!glfwWindowShouldClose(window))
  {
    // Clear screen
    screen.clear();

    // Draw various text samples
    screen.draw_text(font, "HELLO WORLD!", 5, 5, 1);
    screen.draw_text(font, "Text Renderer", 5, 15, 1);
    screen.draw_text(font, "0123456789", 5, 25, 1);
    screen.draw_text(font, "ABC xyz 123", 5, 35, 1);
    screen.draw_text(font, "!@#$%^&*()", 5, 45, 1);

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
