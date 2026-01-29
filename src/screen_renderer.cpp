#include "screen_renderer.hpp"
#include <vector>
#include <cstring>
#include <iostream>

namespace screen_renderer
{

// Vertex shader source - SIMPLIFIED (Pass-through)
const char *vertex_shader_source = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

// uniform float aspectRatio; // Removed

out vec2 TexCoord;

void main()
{
  // Pass-through coordinates. 
  // We rely on the Viewport and Blit to handle aspect ratio.
  gl_Position = vec4(aPos, 0.0, 1.0);
  TexCoord = aTexCoord;
}
)";

// Fragment shader source
const char *fragment_shader_source = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform vec3 backgroundColor;
uniform vec3 pixelColor;

void main()
{
  float pixel = texture(screenTexture, TexCoord).r;
  FragColor = vec4(mix(backgroundColor, pixelColor, pixel), 1.0);
}
)";

screen_renderer_t::screen_renderer_t(float bg_r, float bg_g, float bg_b, float pixel_r, float pixel_g, float pixel_b) : m_vao(0), m_vbo(0), m_texture(0), m_framebuffer(0), m_render_texture(0), m_render_width(512), m_render_height(256)
{
  m_bg_color[0] = bg_r;
  m_bg_color[1] = bg_g;
  m_bg_color[2] = bg_b;

  m_pixel_color[0] = pixel_r;
  m_pixel_color[1] = pixel_g;
  m_pixel_color[2] = pixel_b;

  initialize_opengl();
}

screen_renderer_t::~screen_renderer_t()
{
  glDeleteVertexArrays(1, &m_vao);
  glDeleteBuffers(1, &m_vbo);
  glDeleteTextures(1, &m_texture);
  glDeleteTextures(1, &m_render_texture);
  glDeleteFramebuffers(1, &m_framebuffer);
}

auto screen_renderer_t::render(const screen_t &screen) -> void
{
  // Capture current viewport (window size) BEFORE changing it for FBO
  GLint window_viewport[4];
  glGetIntegerv(GL_VIEWPORT, window_viewport);

  // 1. Update the texture with new screen data
  update_texture(screen);

  // Ensure no scissor test affects us
  glDisable(GL_SCISSOR_TEST);

  // 2. Render to off-screen framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
  glViewport(0, 0, m_render_width, m_render_height);

  // Clear to Background Color
  glClearColor(m_bg_color[0], m_bg_color[1], m_bg_color[2], 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  m_shader->use();
  m_shader->set_uniform_vec3("backgroundColor", m_bg_color[0], m_bg_color[1], m_bg_color[2]);
  m_shader->set_uniform_vec3("pixelColor", m_pixel_color[0], m_pixel_color[1], m_pixel_color[2]);
  m_shader->set_uniform_int("screenTexture", 0);
  // m_shader->set_uniform_float("aspectRatio", 1.0f); // Removed

  glBindVertexArray(m_vao);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);

  // 3. Blit (copy) the framebuffer to the default framebuffer (screen)

  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // Default framebuffer

  int window_width = window_viewport[2];
  int window_height = window_viewport[3];

  float screen_aspect = static_cast<float>(m_render_width) / static_cast<float>(m_render_height);
  float window_aspect = static_cast<float>(window_width) / static_cast<float>(window_height);

  int target_width, target_height;
  int offset_x = 0, offset_y = 0;

  if (window_aspect > screen_aspect)
  {
    target_height = window_height;
    target_width = static_cast<int>(window_height * screen_aspect);
    offset_x = (window_width - target_width) / 2;
  }
  else
  {
    target_width = window_width;
    target_height = static_cast<int>(window_width / screen_aspect);
    offset_y = (window_height - target_height) / 2;
  }

  glBlitFramebuffer(0, 0, m_render_width, m_render_height, offset_x, offset_y, offset_x + target_width, offset_y + target_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Restore viewport
  glViewport(window_viewport[0], window_viewport[1], window_viewport[2], window_viewport[3]);
}

auto screen_renderer_t::set_background_color(float r, float g, float b) -> void
{
  m_bg_color[0] = r;
  m_bg_color[1] = g;
  m_bg_color[2] = b;
}

auto screen_renderer_t::set_pixel_color(float r, float g, float b) -> void
{
  m_pixel_color[0] = r;
  m_pixel_color[1] = g;
  m_pixel_color[2] = b;
}

auto screen_renderer_t::get_texture_id() const -> GLuint
{
  return m_render_texture;
}

auto screen_renderer_t::initialize_opengl() -> void
{
  // Create shader
  m_shader = std::make_unique<shader_t>(vertex_shader_source, fragment_shader_source);

  // Quad vertices (position + texture coordinates)
  float vertices[] = {
      // positions   // texture coords
      -1.0f, 1.0f,  0.0f, 0.0f, // top left
      -1.0f, -1.0f, 0.0f, 1.0f, // bottom left
      1.0f,  -1.0f, 1.0f, 1.0f, // bottom right

      -1.0f, 1.0f,  0.0f, 0.0f, // top left
      1.0f,  -1.0f, 1.0f, 1.0f, // bottom right
      1.0f,  1.0f,  1.0f, 0.0f  // top right
  };

  // Create VAO and VBO
  glGenVertexArrays(1, &m_vao);
  glGenBuffers(1, &m_vbo);

  glBindVertexArray(m_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Texture coordinate attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);

  // Create screen data texture
  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  // Create render texture (for framebuffer)
  glGenTextures(1, &m_render_texture);
  glBindTexture(GL_TEXTURE_2D, m_render_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_render_width, m_render_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  // Create framebuffer
  glGenFramebuffers(1, &m_framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_render_texture, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    std::cerr << "Framebuffer is not complete!" << std::endl;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

auto screen_renderer_t::update_texture(const screen_t &screen) -> void
{
  const auto &pixel_data = screen.get_data();
  size_t width = screen.get_width();
  size_t height = screen.get_height();

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // Convert vector<bool> to byte array
  std::vector<unsigned char> texture_data(width * height);
  for (size_t i = 0; i < pixel_data.size(); ++i)
  {
    texture_data[i] = pixel_data[i] ? 255 : 0;
  }

  glBindTexture(GL_TEXTURE_2D, m_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, texture_data.data());
  glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace screen_renderer
