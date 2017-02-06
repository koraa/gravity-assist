#pragma once

#include <glm/gtc/type_precision.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

namespace gassist {

// DEFAULT TYPES //////////////

typedef unsigned int uint;

/// Our default floating point type; we would use float
/// but we want to (sort of) keep configurable
typedef float fl;

typedef glm::mat4  mat4;
typedef glm::fvec2 vec2;
typedef glm::fvec3 vec3;
typedef glm::fvec4 vec4;

/// Type signifying empty values; e.g. used for
/// specifically constructing an empty object to
/// move into later
struct empty_t {};

/// Value of empty_t
constexpr empty_t empty;

// CONSTANTS ///////////////////

const fl pi  = glm::pi<fl>(),
         tau = 2*pi;
mat4 identity{1.0f};


// HELPERS /////////////////////

/// Generates a translation matrix based on a vector
mat4 translate(const vec3 &v) {
  return glm::translate(identity, v);
}

mat4 translate(float x, float y, float z) {
  return translate(vec3(x, y, z));
}

mat4 scale(const vec3 &v) {
  return glm::scale(identity, v);
}

mat4 scale(float x, float y, float z) {
  return scale(vec3(x, y, z));
}

/// Generates a rotation transform matrix based on an angle
/// and a rotation axis
mat4 rotate(const fl angle, const vec3 &axis) {
  return glm::rotate(identity, angle, axis);
}

/// Applies a 4d matrix transformation to a 3d vector
/// (converts the 3d vector to 4d by setting w=1 and
/// then drops w again)
vec3 operator*(const mat4 &m, const vec3 &v) {
  auto r = (m * vec4{v, 1});
  return {r.x, r.y, r.z};
}


// DATA STRUCTURES //////////////////

/// Position vector (normal Cartesian coordinates)
///
/// Specializable template; the default implementation
/// just uses x.pos()
///
/// The return value must support conversion to and
/// assignment from vec3 (like a vec3&)
template<typename T>
vec3& pos(T &x) {
  return x.pos();
}


/// Orientation; what the object is facing towards
/// relatively to itself. (e.g. (0, 1, 0) would be
/// facing upwards)
///
/// Specializable template; the default implementation
/// just uses x.focus().
///
/// The return value must support conversion to and
/// assignment from vec3 (like a vec3&)
template<typename T>
vec3& focus(T &x) {
  return x.focus();
}

/// How the world is rotated; e.g. focus=(0, 0, -1)
/// roll=0; would be facing front as normal, while
/// focus=(0, 0, -1) roll=pi would be facing front
/// with the camera standing on it's head
///
/// Specializable template; the default implementation
/// just uses x.roll()
///
/// The return value must support conversion to and
/// assignment from float (like a float&)
template<typename T>
fl& roll(T &x) {
  return x.roll();
}

/// Just a simple struct that holds position, focus
/// and roll of any object
struct location {
  vec3 pos_;
  vec3 focus_;
  fl roll_;

  vec3& pos()   { return pos_; }
  vec3& focus() { return focus_; }
  fl& roll() { return roll_; }
};

} // ns gassist
