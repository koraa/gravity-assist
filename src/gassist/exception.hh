#pragma once

#include <cstring>

#include <exception>
#include <string>
#include <sstream>

namespace gassist {

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

} // ns gassist
