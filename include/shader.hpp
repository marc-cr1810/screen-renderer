#pragma once

#include <GL/glew.h>
#include <string>

namespace screen_renderer
{

class shader_t
{
public:
  // Functions
  shader_t(const std::string &vertex_source, const std::string &fragment_source);
  ~shader_t();
  auto use() const -> void;
  auto get_program_id() const -> GLuint;
  auto set_uniform_mat4(const std::string &name, const float *value) const -> void;
  auto set_uniform_vec3(const std::string &name, float x, float y, float z) const -> void;
  auto set_uniform_int(const std::string &name, int value) const -> void;
  auto set_uniform_float(const std::string &name, float value) const -> void;

private:
  // Functions
  auto compile_shader(GLenum type, const std::string &source) -> GLuint;
  auto link_program(GLuint vertex_shader, GLuint fragment_shader) -> GLuint;

  // Variables
  GLuint m_program_id;
};

} // namespace screen_renderer
