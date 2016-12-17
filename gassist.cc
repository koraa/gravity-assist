#include "wrap.hh"

int main() {
  using namespace gassist;

  wrap::window w{"Gravity Assist"};

  glClearColor(0.0f, 0.0f, 0.1f, 0.0f);
  while (!w.should_close()) {
	  glClear(GL_COLOR_BUFFER_BIT);
    w.swap_buffers();
    glfwWaitEvents();
  }

  return 0;
}
