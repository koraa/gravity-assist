#pragma once

#include <utility>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "gassist/exception.hh"
#include "gassist/util.hh"

namespace gassist::asset {

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

// TODO: Support a load path
// TODO: Support multiple sources
// TODO: Use async loading
// TODO: Put asset loading on extra thread(s)

/// Loads an opengl program from a directory
gl::program load_gl_program(const std::string &dir) {
  mapped_file sfrag{dir + "/main.frag.glsl"},
              svert{dir + "/main.vert.glsl"};
  gl::shader vert = gl::make_vertex_shader(svert.data(), svert.size()),
             frag = gl::make_fragment_shader(sfrag.data(), sfrag.size());

  return gl::program{frag, vert};
}

};
