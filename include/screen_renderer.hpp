#pragma once

#include "screen.hpp"
#include "shader.hpp"
#include <GL/glew.h>
#include <memory>

namespace screen_renderer
{

class screen_renderer_t
{
public:
  // Functions
  screen_renderer_t(float bg_r = 0.0f, float bg_g = 0.5f, float bg_b = 1.0f, float pixel_r = 0.0f, float pixel_g = 0.0f, float pixel_b = 0.0f);
  ~screen_renderer_t();
  auto render(const screen_t &screen) -> void;
  auto set_background_color(float r, float g, float b) -> void;
  auto set_pixel_color(float r, float g, float b) -> void;
  auto get_texture_id() const -> GLuint;

private:
  // Functions
  auto initialize_opengl() -> void;
  auto update_texture(const screen_t &screen) -> void;

  // Variables
  std::unique_ptr<shader_t> m_shader;
  GLuint m_vao;
  GLuint m_vbo;
  GLuint m_texture;        // Raw screen data texture
  GLuint m_framebuffer;    // Framebuffer for rendering
  GLuint m_render_texture; // Final rendered texture with colors
  int m_render_width;
  int m_render_height;
  float m_bg_color[3];
  float m_pixel_color[3];
};

} // namespace screen_renderer
