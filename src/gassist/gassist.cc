#include <cmath>

#include <thread>
#include <atomic>

#include <epoxy/gl.h>

#include <softwear/thread_pool.hpp>

#include "gassist/util.hh"

#include "gassist/wrap_glfw.hh"
#include "gassist/wrap_gl.hh"

#include "gassist/asset.hh"

using namespace gassist;

// TODO: Use ranges/iterators
std::array<vec3, 3> tri_verts(vec3 a, vec3 b, vec3 c) {
  return {a, b, c};
}

std::array<vec3, 3*2> quad_verts(vec3 a, vec3 b, vec3 c, vec3 d) {
  return {a, b, c,   b, c, d};
}

gl::mesh make_rectangle() {
  auto front = quad_verts(
    {+1, +1, 0}, {+1, -1, 0}, {-1, +1, 0}, {-1, -1, 0});

  return gl::mesh{front};
}

/// Program state that is shared between threads
struct shared_state {
  //// BASIC VARIABLES ////

  /// The window we're drawing in
  glfw::window win{"Gravity Assist"};

  /// The size of the window W
  vec2 win_size{1,1};
  // NOTE: Storing this in the state and loading it in
  // the input thread because we don't know what kind of
  // performance input this has.
  // NOTE: We should move this to a store in glfw::window
  // and update via events, but the glfw events are not
  // very convenient (won't allow passing the address of
  // the c++ wrapper to the callback)
  // NOTE: Initializing not to zero in order to avoid
  // division by zero errors on start

  /// Indicates to the drawing thread that a resize of the
  /// viewport is necessary
  std::atomic<bool> opengl_needs_resize{false};
  // TODO: Later we should introduce a signaling queue with
  // signals indicating those kinds of events

  /// Set to false to stop the program
  std::atomic<bool> stop{false};

  //// WORLD STATE ////

  /// Where the camera is at
  location cam{
    {0,  10,  8},
    {0, -10, -8},
    0
  };

  //// SETTINGS ////

  // y axis field of view in degrees
  float fov = 110;
};

////////////// DRAWING ///////////////////////

void draw_thr(shared_state &s) {
  // We should have something nicer for this.
  // Window should implicitly create the context
  // and allow it to be used from another thread
  s.win.make_gl_context();

  gl::program default_prog = asset::load_gl_program("default_prog");
  // TODO: We need a generic, compile time soluition
  // for representing shader parameters
  GLint param_mvp = glGetUniformLocation(default_prog.id(), "mvp");

  gl::mesh tri = make_rectangle();

  // TODO: Error handling: is the extension loaded?
  glfwSwapInterval(1);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  mat4 vp; // View-projection matrix

  auto draw = [&](auto &obj, const vec3 &pos) {
    // TODO: We need a more generic way of expressing this
    mat4 mvp = vp * translate(pos);
    glUniformMatrix4fv(param_mvp, 1, GL_FALSE, &mvp[0][0]);
    obj.draw();
  };

  default_prog.use();
  while (!s.stop) {
    // Make a snapshot of the state
    // (just in case it changes concurrently)
    location cam = s.cam;

    // Adjust the view/projection matrix to accomodate
    // position, fov and window size updates.
    // TODO: Use the roll component of the vector
    mat4 persp = glm::perspective(
                          tau*s.fov/360,
                          s.win_size.x / s.win_size.y,
                          0.01f, 1000.0f);
    vec3 up = rotate(roll(cam), vec3{0, 0, -1})
            * vec3{0, 1, 0};
    mat4 look = glm::lookAt(pos(cam),
                            pos(cam) + focus(cam),
                            up);
    vp = persp * look;

    if (s.opengl_needs_resize) {
      glViewport(0, 0, (int)s.win_size.x, (int)s.win_size.y);
      glScissor(0, 0, (int)s.win_size.x, (int)s.win_size.y);
      s.opengl_needs_resize = false;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    draw(tri, {0, 0, 0});

    s.win.swap_buffers();

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
  // TODO: This code is not very pretty
  glm::tvec2<double> mousepos, mouse_lastpos, mouse_delta;


  while (!s.stop) {
    glfwWaitEvents();

    {
      auto nu_size = s.win.size();
      if (nu_size != s.win_size)
        s.opengl_needs_resize = true;
      s.win_size = nu_size;
    }

    bool mousel = glfwGetMouseButton(s.win.glfw_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS,
         mousem = glfwGetMouseButton(s.win.glfw_window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS,
         shift  = glfwGetKey(s.win.glfw_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

    mouse_lastpos = mousepos;
    glfwGetCursorPos(s.win.glfw_window, &mousepos.x, &mousepos.y);
    mouse_delta = mousepos - mouse_lastpos;

    //// WINDOW CLOSED ////
    s.stop = s.win.should_close();

    //// CAMERA NAVIGATION ////

    if (mousem || (mousel && shift)) { // zoom
      float mag = mouse_delta.y - mouse_delta.x;
      pos(s.cam) *= std::pow(10, mag/500);

    } else if (mousel) {
      auto alt_axis =
          rotate(90, vec3{0, 1, 0})
        * glm::normalize(pos(s.cam) * vec3{1, 0, 1});

      // TODO: Not locking here. This seems a bit dangerous,
      // but apparently works

      pos(s.cam) = rotate(-mouse_delta.x/40, {0, 1, 0})
                 * rotate(-mouse_delta.y/40, alt_axis)
                 * pos(s.cam);

      // Note: We're orbiting around 0, 0, 0
      focus(s.cam) = vec3{0, 0, 0} - pos(s.cam);
    }
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
