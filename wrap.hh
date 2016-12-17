/// Provides basic wrappers for c libraries like
/// glfw or libepoxy

#include <epoxy/gl.h>
#include <epoxy/glx.h>

#include <GLFW/glfw3.h>

#include <softwear/phoenix_ptr.hpp>

namespace gassist::wrap {

////////// GLOBAL INITIALIZATION //////////////

namespace intern {

struct wrap_subsystem_t {
  wrap_subsystem_t() {
    // TODO: Handle error
    glfwInit();
  }

  ~wrap_subsystem_t() {
    glfwTerminate();
  }
};

extern softwear::phoenix_ptr<wrap_subsystem_t> wrap_subsystem;

} // ns intern

/// Makes sure that libepoxy, glfw, ... are all
/// initialized.
///
/// Automatically terminates those libs when the return
/// value is destroyed.
///
/// Can be held multiple times in multiple threads.
/// Thread safe.
inline auto init()
    -> decltype(intern::wrap_subsystem.lock()) {
  return intern::wrap_subsystem.lock();
}

////////// WINDOW MANAGEMENT & INIT ///////////

/// Opens a window using glfw;
///
/// Must only be used on main thread.
class window {
  decltype(init()) sys_ = init();
public:
  GLFWwindow* glfw_window;

  window(const int w, const int h,
         const std::string &title) noexcept {
    // TODO: Again – error handling
    glfwWindowHint(GLFW_RESIZABLE, true);
    glfwWindowHint(GLFW_VISIBLE,   true);
    glfwWindowHint(GLFW_FOCUSED,   true);
    glfwWindowHint(GLFW_MAXIMIZED, true);
    glfw_window = glfwCreateWindow(w, h, title.c_str(),
                            nullptr, nullptr);
  }

  window(const std::string &title="")
      : window{640, 480, title} {}

  ~window() {
    glfwDestroyWindow(glfw_window);
  }

  void make_gl_context() {
    glfwMakeContextCurrent(glfw_window);
  }

  void swap_buffers() {
    glfwSwapBuffers(glfw_window);
  }

  bool should_close() {
    return glfwWindowShouldClose(glfw_window);
  }
};

////////// RENDERING //////////////////////////

} // ns gassist::glfw
