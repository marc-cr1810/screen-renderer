#include "shader.hpp"
#include <iostream>
#include <vector>

namespace screen_renderer
{

shader_t::shader_t(const std::string &vertex_source, const std::string &fragment_source)
{
  GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source);
  GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
  m_program_id = link_program(vertex_shader, fragment_shader);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
}

shader_t::~shader_t()
{
  glDeleteProgram(m_program_id);
}

auto shader_t::use() const -> void
{
  glUseProgram(m_program_id);
}

auto shader_t::get_program_id() const -> GLuint
{
  return m_program_id;
}

auto shader_t::set_uniform_mat4(const std::string &name, const float *value) const -> void
{
  GLint location = glGetUniformLocation(m_program_id, name.c_str());
  if (location != -1)
  {
    glUniformMatrix4fv(location, 1, GL_FALSE, value);
  }
}

auto shader_t::set_uniform_vec3(const std::string &name, float x, float y, float z) const -> void
{
  GLint location = glGetUniformLocation(m_program_id, name.c_str());
  if (location != -1)
  {
    glUniform3f(location, x, y, z);
  }
}

auto shader_t::set_uniform_int(const std::string &name, int value) const -> void
{
  GLint location = glGetUniformLocation(m_program_id, name.c_str());
  if (location != -1)
  {
    glUniform1i(location, value);
  }
}

auto shader_t::set_uniform_float(const std::string &name, float value) const -> void
{
  GLint location = glGetUniformLocation(m_program_id, name.c_str());
  if (location != -1)
  {
    glUniform1f(location, value);
  }
}

auto shader_t::compile_shader(GLenum type, const std::string &source) -> GLuint
{
  GLuint shader = glCreateShader(type);
  const char *source_cstr = source.c_str();
  glShaderSource(shader, 1, &source_cstr, nullptr);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    GLint log_length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
    std::vector<char> log(log_length);
    glGetShaderInfoLog(shader, log_length, nullptr, log.data());
    std::cerr << "Shader compilation failed: " << log.data() << std::endl;
  }

  return shader;
}

auto shader_t::link_program(GLuint vertex_shader, GLuint fragment_shader) -> GLuint
{
  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success)
  {
    GLint log_length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
    std::vector<char> log(log_length);
    glGetProgramInfoLog(program, log_length, nullptr, log.data());
    std::cerr << "Program linking failed: " << log.data() << std::endl;
  }

  return program;
}

} // namespace screen_renderer
