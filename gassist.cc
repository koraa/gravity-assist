#include <cstring>

#include <thread>
#include <atomic>
#include <utility>
#include <sstream>
#include <functional>
#include <initializer_list>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <epoxy/gl.h>

#include <glm/gtc/type_precision.hpp>

#include <softwear/thread_pool.hpp>

#include "wrap.hh"

using namespace gassist;

// TODO: Use specific error handling
class msg_exception : public std::exception {
  std::string what_;
public:
  msg_exception(const std::string &str) : what_{str} {}
  virtual const char* what() const noexcept {
      return what_.c_str();
  }
};

class errno_exception {
  std::string what_;
public:
  errno_exception(int no) {
    std::stringstream s;
    s << "Error " << no << ": " << strerror(no) << ".";
    what_ = s.str();
  }
  errno_exception() : errno_exception{errno} {}
  virtual const char* what() const noexcept {
      return what_.c_str();
  }
};

struct empty_t {} empty;

struct open_fd {
private:
  int _fd = -1;

  bool good() const { return _fd >= 0; }

public:

  open_fd(empty_t) {} // For moving into

  open_fd(const std::string &f) {
    _fd = ::open(f.c_str(), O_RDONLY);
    if (!good()) throw errno_exception{};
  }

  ~open_fd() {
    if (good())
      ::close(_fd);
  }

  int fd() const { return _fd; }

  open_fd(const open_fd&) = delete;
  open_fd& operator =(const open_fd&otr) = delete;

  open_fd(open_fd&& otr) { std::swap(*this, otr); }
  open_fd& operator=(open_fd&& otr) {
    std::swap(*this, otr);
    return *this;
  }

  void swap(open_fd &otr) {
    std::swap(_fd, otr._fd);
  }
};

struct mapped_file {
private:
  open_fd _fd;
  char *_data = (char*)MAP_FAILED;
  size_t _size;

  bool good() const { return _data != MAP_FAILED; }
public:
  mapped_file(empty_t) : _fd{empty} {} // For moving into

  mapped_file(const std::string &f) : _fd{f} {
    struct stat s;
    fstat(fd(), &s);
    _size = s.st_size;

    _data = (char*) ::mmap(nullptr, _size, PROT_READ,
                           MAP_PRIVATE, fd(), 0);
    if (!good())
      throw errno_exception{};
  }

  ~mapped_file() {
    if (good()) ::munmap(_data, _size);
  }

  int fd() const { return _fd.fd(); }

  size_t size() const { return _size; }
  char* data() { return _data; }
  char* begin() { return data(); }
  char* end() { return begin() + size(); }

  mapped_file(const mapped_file&) = delete;
  mapped_file& operator =(const mapped_file&otr) = delete;

  mapped_file(mapped_file&& otr) : _fd{empty} {
    swap(otr);
  }

  mapped_file& operator=(mapped_file&& otr) {
    swap(otr);
    return *this;
  }

  void swap(mapped_file &otr) {
    std::swap(_fd, otr._fd);
    std::swap(_data, otr._data);
    std::swap(_size, otr._size);
  }
};

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

template<typename... Args>
shader vertex_shader(Args&&... args) {
  return shader{GL_VERTEX_SHADER, std::forward<Args>(args)...};
}

template<typename... Args>
shader fragment_shader(Args&&... args) {
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
class gl_program {
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
  gl_program(const R &shaders) {
    from_range(shaders);
  }

  gl_program(const std::initializer_list<std::reference_wrapper<shader>> &l) {
    from_range(l);
  }

  ~gl_program() {
    glDeleteProgram(id());
  }

  GLuint id() const { return _id; }

  void _use() {
    glUseProgram(id());
  }

  gl_program(const gl_program&) = delete;
  gl_program& operator =(const gl_program&otr) = delete;

  gl_program(gl_program&& otr) { swap(otr); }
  gl_program& operator=(gl_program&& otr) {
    swap(otr);
    return *this;
  }

  void swap(gl_program &otr) {
    std::swap(_id, otr._id);
  }
};

// TODO: Support a load path
// TODO: Support multiple sources
// TODO: Use async loading
// TODO: Put asset loading on extra thread(s)

/// Loads an opengl program from a directory
gl_program load_gl_program(const std::string &dir) {
  mapped_file sfrag{dir + "/main.frag.glsl"},
              svert{dir + "/main.vert.glsl"};
  shader vert = vertex_shader(svert.data(), svert.size()),
         frag = fragment_shader(sfrag.data(), sfrag.size());

  return gl_program{frag, vert};
}

/// Uses a gl_program while the lambda F is executing
template<typename T, typename F>
void with_gl_prog(T &p, F f) {
  p._use();
  f();
}

struct mesh {
  GLuint id_vertex_array;
	GLuint id_vertex_buffer;
  size_t no_vertices;

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

template<typename T>
void draw(T &o) {
  o._draw();
}

mesh make_triangle() {
  std::array<float, 9> verts{
    -1, -1, 0,
     1, -1, 0,
     0,  1, 0};

  return mesh{verts};
}

/// Program state that is shared between threads
struct shared_state {
 /// The window we're drawing in
  wrap::window w{"Gravity Assist"};

  /// Set to false to stop the program
  std::atomic<bool> stop{false};
};

////////////// DRAWING ///////////////////////

void draw_thr(shared_state &s) {
  // We should have something nicer for this.
  // Window should implicitly create the context
  // and allow it to be used from another thread
  s.w.make_gl_context();

  gl_program default_prog = load_gl_program("default_prog");
  mesh tri = make_triangle();

  // TODO: Error handling: is the extension loaded?
  glfwSwapInterval(1);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  while (!s.stop) {
    //glClear(GL_COLOR_BUFFER_BIT);

    with_gl_prog(default_prog, [&](){
      draw(tri);
    });

    s.w.swap_buffers();

    // To save CPU we synchronize drawing the frame
    // with the Frame rate. This saves some CPU for
    // now, but we may need a better solution later
    glFinish();
  }
}

////////////// INPUT /////////////////////////

// NOTE: This necessarily must be placed in the
// main thread
void input_thr(shared_state &s) {
  while (!s.stop) {
    glfwWaitEvents();
    s.stop = s.w.should_close();
  }
}

////////////// MAIN //////////////////////////

int main() {
  shared_state state;

  softwear::thread_pool painters(1, draw_thr, state);
  input_thr(state);

  // Note: All threads will exit and be joined
  // automatically

  return 0;
}
