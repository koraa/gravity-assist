#include <thread>
#include <atomic>

#include <epoxy/gl.h>

#include <softwear/thread_pool.hpp>

#include "gassist/wrap_glfw.hh"
#include "gassist/wrap_gl.hh"

#include "gassist/asset.hh"

using namespace gassist;

gl::mesh make_triangle() {
  std::array<float, 9> verts{
    -1, -1, 0,
     1, -1, 0,
     0,  1, 0};

  return gl::mesh{verts};
}

/// Program state that is shared between threads
struct shared_state {
 /// The window we're drawing in
  glfw::window w{"Gravity Assist"};

  /// Set to false to stop the program
  std::atomic<bool> stop{false};
};

////////////// DRAWING ///////////////////////

void draw_thr(shared_state &s) {
  // We should have something nicer for this.
  // Window should implicitly create the context
  // and allow it to be used from another thread
  s.w.make_gl_context();

  gl::program default_prog = asset::load_gl_program("default_prog");
  gl::mesh tri = make_triangle();

  // TODO: Error handling: is the extension loaded?
  glfwSwapInterval(1);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  while (!s.stop) {
    //glClear(GL_COLOR_BUFFER_BIT);

    gl::with_program(default_prog, [&](){
        gl::draw(tri);
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
