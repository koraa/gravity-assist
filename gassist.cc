#include <thread>
#include <atomic>

#include <epoxy/gl.h>

#include "oglplus/context.hpp"

#include "softwear/thread_pool.hpp"

#include "wrap.hh"

using namespace gassist;

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
  oglplus::Context gl;

  // TODO: Error handling: is the extension loaded?
  glfwSwapInterval(1);
  gl.ClearColor(0.0f, 0.0f, 0.1f, 0.0f);

  while (!s.stop) {
    gl.Clear().ColorBuffer();
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
