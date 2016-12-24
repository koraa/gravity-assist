#pragma once

#include <initializer_list>
#include <sstream>
#include <functional>
#include <utility>

#include <epoxy/gl.h>

#include "gassist/exception.hh"

namespace gassist::gl {

// represents a single, compile shader
// (could be a vertex shader, pixel shader, ...)
//
// Can not be used on it's own. Use the shader_program.
class shader {
  GLenum _type;
  GLuint _id;

public:
  shader(const GLenum type, const char *s, size_t len)
      : _type{type}, _id{glCreateShader(_type)} {
    int _len = len; // TODO: Check overflow
    glShaderSource(id(), 1, &s, &_len);
    glCompileShader(id());

    GLint success;
	  glGetShaderiv(id(), GL_COMPILE_STATUS, &success);
    if (!success) {
      // TODO: Reduce the numbers of copies here
      int log_len;
	    glGetShaderiv(id(), GL_INFO_LOG_LENGTH, &log_len);
      std::vector<char> log;
      log.resize(log_len+1);
      glGetShaderInfoLog(id(), log.size(), nullptr, log.data());

      std::stringstream msg;
      msg << "Failed compiling shader: ";
      msg.write(log.data(), log_len);

      // TODO: Handle in a better way
      glDeleteShader(_id);

      throw msg_exception{msg.str()};

    }
  }

  ~shader() {
    glDeleteShader(_id);
  }

  GLenum type() const { return _type; }
  GLuint id() const { return _id; }

  shader(const shader&) = delete;
  shader& operator =(const shader&otr) = delete;

  shader(shader&& otr) { swap(otr); }
  shader& operator=(shader&& otr) {
    swap(otr);
    return *this;
  }

  void swap(shader &otr) {
    std::swap(_type, otr._type);
    std::swap(_id, otr._id);
  }
};

// Shorthand for creating a shader with
// type=GL_VERTEX_SHADER
template<typename... Args>
shader make_vertex_shader(Args&&... args) {
  return shader{GL_VERTEX_SHADER, std::forward<Args>(args)...};
}

// Shorthand for creating a shader with
// type=GL_FRAGMENT_SHADER
template<typename... Args>
shader make_fragment_shader(Args&&... args) {
  return shader{GL_FRAGMENT_SHADER, std::forward<Args>(args)...};
}

/// A compiled program; must not be recompiled, build another
/// program instead. Reflection features of opengl programs
/// must not be used.
///
/// The purpose of this is to present a representation of a
/// program that has already been linked and resides in gpu
/// memory without consuming the resources (memory) of any
/// attached shader objects.
///
/// This allows us to decouple reflection/linking capabilities
/// from the actual shaders that are in use. Reflection/linking
/// should be handled by wrapper classes.
class program {
  GLuint _id = glCreateProgram();

  template<typename R>
  void from_range(const R &shaders) {
    for (shader &s : shaders) glAttachShader(id(), s.id());
    glLinkProgram(id());
    for (shader &s : shaders) glDetachShader(id(), s.id());

    GLint success;
	  glGetProgramiv(id(), GL_LINK_STATUS, &success);
    if (!success) {
      // TODO: Reduce the numbers of copies here
      int log_len;
	    glGetProgramiv(id(), GL_INFO_LOG_LENGTH, &log_len);
      std::vector<char> log;
      log.resize(log_len+1);
      glGetProgramInfoLog(id(), log.size(), nullptr, log.data());

      std::stringstream msg;
      msg << "Failed linking program: ";
      msg.write(log.data(), log_len);

      // TODO: Handle this in a better way
      glDeleteProgram(id());

      throw msg_exception{msg.str()};

    }
  }
public:
  template<typename R>
  program(const R &shaders) {
    from_range(shaders);
  }

  program(const std::initializer_list<std::reference_wrapper<shader>> &l) {
    from_range(l);
  }

  ~program() {
    glDeleteProgram(id());
  }

  GLuint id() const { return _id; }

  void _use() {
    glUseProgram(id());
  }

  program(const program&) = delete;
  program& operator =(const program&otr) = delete;

  program(program&& otr) { swap(otr); }
  program& operator=(program&& otr) {
    swap(otr);
    return *this;
  }

  void swap(program &otr) {
    std::swap(_id, otr._id);
  }
};

/// Uses a program while the lambda F is executing
/// Essentially a wrapper for glUseProgram
template<typename T, typename F>
void with_program(T &p, F f) {
  p._use();
  f();
}

/// Basic wrapper for a 3d mesh/model.
/// Takes a range of vertices and takes care of uploading
/// them to the GPU and painting them
///
/// Paint with draw()
class mesh {
  GLuint id_vertex_array;
	GLuint id_vertex_buffer;
  size_t no_vertices;

public:
  template<typename R>
  mesh(const R &vertices) noexcept {
	  glGenVertexArrays(1, &id_vertex_array);
    glBindVertexArray(id_vertex_array);

    glGenBuffers(1, &id_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, id_vertex_buffer);

    // NOTE: Assuming this is contiguous
    no_vertices = vertices.size() / 3;
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size()*sizeof(typename R::value_type),
                 vertices.begin(), GL_STATIC_DRAW);
  }

  ~mesh() {
    glDeleteBuffers(1, &id_vertex_buffer);
	  glDeleteVertexArrays(1, &id_vertex_array);
  }

  /// Draws this mesh. You should probably use draw() instead
  void _draw() {
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, id_vertex_buffer);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			no_vertices,        // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, no_vertices);

		glDisableVertexAttribArray(0);
  }
};

/// Function that can draw any kind of object.
/// This really just forwards to the argument._draw()
template<typename T>
void draw(T &o) {
  o._draw();
}

} // ns gassist::gl
