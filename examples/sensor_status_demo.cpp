#include "screen.hpp"
#include "screen_renderer.hpp"
#include "font.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>

using namespace screen_renderer;

// Simulated sensor state
struct sensor_state_t
{
  float battery_percent = 100.0f;
  bool network_connected = true;
  int detected_drones = 0;
  // Signal strength removed as per user request
  float cpu_temp = 45.0f; // Celsius
  uint32_t uptime_seconds = 0;
  bool gps_lock = true;
  double latitude = -33.8688;
  double longitude = 151.2093;

  // History for graph
  std::vector<float> drone_history;
  size_t max_history = 40;

  void update(int count)
  {
    drone_history.push_back((float)count);
    if (drone_history.size() > max_history)
    {
      drone_history.erase(drone_history.begin());
    }
  }
};

// --- Drawing Helpers ---

// Draw a filled rectangle
void draw_filled_rect(screen_t &screen, int x, int y, int w, int h, bool color)
{
  for (int i = 0; i < w; ++i)
  {
    for (int j = 0; j < h; ++j)
    {
      screen.set_pixel(x + i, y + j, color);
    }
  }
}

// Draw a large 3x5 digit scaled up by scale factor
void draw_large_digit(screen_t &screen, int x, int y, int digit, int scale = 3)
{
  // 3x5 font map (0-9)
  // Simple bitmap for 3x5 digits (0-9)
  // 1 = pixel on
  static const bool digits[10][15] = {
      {1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1}, // 0
      {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0}, // 1
      {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1}, // 2
      {1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1}, // 3
      {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1}, // 4
      {1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1}, // 5
      {1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1}, // 6
      {1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1}, // 7
      {1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1}, // 8
      {1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1}, // 9
  };

  if (digit < 0 || digit > 9)
    return;

  const bool *d = digits[digit];
  for (int r = 0; r < 5; ++r)
  {
    for (int c = 0; c < 3; ++c)
    {
      if (d[r * 3 + c])
      {
        draw_filled_rect(screen, x + c * scale, y + r * scale, scale, scale, true);
      }
    }
  }
}

void draw_large_number(screen_t &screen, int x, int y, int number, int scale = 3)
{
  std::string s = std::to_string(number);
  int cursor_x = x;
  for (char c : s)
  {
    if (c >= '0' && c <= '9')
    {
      draw_large_digit(screen, cursor_x, y, c - '0', scale);
      cursor_x += (3 * scale) + scale; // width + spacing
    }
  }
}

// Draw a line graph
void draw_graph(screen_t &screen, int x, int y, int w, int h, const std::vector<float> &data, float min_val, float max_val)
{
  screen.draw_rect(x, y, w, h, true); // Frame

  if (data.empty())
    return;

  int points = data.size();
  float x_step = (float)(w - 2) / (float)(points > 1 ? points - 1 : 1);

  int prev_px = -1;
  int prev_py = -1;

  for (int i = 0; i < points; ++i)
  {
    float val = data[i];
    if (val < min_val)
      val = min_val;
    if (val > max_val)
      val = max_val;

    float norm = 1.0f - ((val - min_val) / (max_val - min_val)); // 0 = top, 1 = bottom
    int py = y + 1 + (int)(norm * (h - 3));
    int px = x + 1 + (int)(i * x_step);

    // Clamp to be safe
    if (px >= x + w - 1)
      px = x + w - 2;

    if (i > 0)
    {
      screen.draw_line(prev_px, prev_py, px, py, true);
    }
    prev_px = px;
    prev_py = py;
  }
}

// Draw horizontal progress bar
void draw_h_bar(screen_t &screen, int x, int y, int w, int h, float val, float max_val)
{
  screen.draw_rect(x, y, w, h, true);
  int fill_w = (int)((val / max_val) * (float)(w - 2));
  if (fill_w > w - 2)
    fill_w = w - 2;
  if (fill_w < 0)
    fill_w = 0;
  draw_filled_rect(screen, x + 1, y + 1, fill_w, h - 2, true);
}

// --- Main Draw Functions ---

void draw_dashboard(screen_t &screen, const font_t &font, const sensor_state_t &sensor)
{
  // --- Header ---
  screen.draw_line(0, 8, 127, 8, true);
  screen.draw_text(font, "SENSOR STATUS", 2, 1, 1);

  // Icons Right
  std::string bat = std::to_string((int)sensor.battery_percent) + "%";
  screen.draw_text(font, bat, 105, 1, 1);

  if (sensor.network_connected)
  {
    screen.draw_text(font, "NET", 85, 1, 1);
  }

  // --- Left Panel: Drone Count ---
  // Area: 0,9 to 64,54
  screen.draw_text(font, "DETECTED", 5, 12, 1);

  // Draw HUGE number centered-ish
  // Number can be 1 or 2 digits usually.
  // 3x5 * scale 6 = 18x30 pixels per digit
  int num_scale = 5;
  if (sensor.detected_drones > 9)
    num_scale = 4; // Shrink slightly for 2 digits

  // Centering logic roughly
  int num_width = (sensor.detected_drones > 9) ? (2 * (3 * num_scale) + num_scale) : (3 * num_scale);
  int start_x = 32 - (num_width / 2);

  draw_large_number(screen, start_x, 22, sensor.detected_drones, num_scale);

  // --- Right Panel: Graphs & Stats ---
  // Area: 64,9 to 127,54
  screen.draw_line(64, 9, 64, 54, true); // Divider

  // Activity Graph
  screen.draw_text(font, "ACTIVITY", 68, 12, 1);
  // Graph area: 68, 20 to 124, 40 (20px high)
  // Max 12 drones as per simulation
  draw_graph(screen, 68, 20, 56, 20, sensor.drone_history, 0.0f, 15.0f);

  // CPU Temp Bar
  screen.draw_text(font, "CPU", 68, 44, 1);
  draw_h_bar(screen, 88, 44, 36, 6, sensor.cpu_temp, 90.0f);

  // --- Footer ---
  screen.draw_line(0, 55, 127, 55, true);
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(4) << sensor.latitude << "," << sensor.longitude;
  screen.draw_text(font, oss.str(), 2, 57, 1);

  if (!sensor.gps_lock)
  {
    screen.draw_text(font, "NO FIX", 100, 57, 1);
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
  GLFWwindow *window = glfwCreateWindow(800, 400, "Drone Detection Sensor Status", nullptr, nullptr);
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

  // Create screen and renderer (128x64 OLED display, blue on black)
  screen_t screen(128, 64);
  screen_renderer_t renderer(0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f); // Default Blue

  // Create font
  font_t font;

  // Initialize sensor state
  sensor_state_t sensor;

  double last_time = glfwGetTime();
  float animation_time = 0.0f;

  while (!glfwWindowShouldClose(window))
  {
    double current_time = glfwGetTime();
    float delta_time = static_cast<float>(current_time - last_time);
    last_time = current_time;
    animation_time += delta_time;

    // Simulate sensor state
    sensor.uptime_seconds = static_cast<uint32_t>(current_time);
    sensor.battery_percent = 100.0f - (current_time / 10.0f);
    if (sensor.battery_percent < 0.0f)
      sensor.battery_percent = 0.0f;

    sensor.detected_drones = static_cast<int>(std::abs(std::sin(animation_time * 0.2f)) * 12); // Range 0-12
    sensor.update(sensor.detected_drones);

    sensor.cpu_temp = 45.0f + std::sin(animation_time * 0.3f) * 10.0f;
    sensor.network_connected = (static_cast<int>(animation_time) % 10) < 8;

    screen.clear();

    // Draw Dashboard
    draw_dashboard(screen, font, sensor);

    // Render
    glClear(GL_COLOR_BUFFER_BIT);
    renderer.render(screen);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
