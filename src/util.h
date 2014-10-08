// This file defines some miscellaneous utilities to be used throughout code.

#ifndef UTIL_H_
#define UTIL_H_

#include <memory>
#include <utility>

// Google's copy-constructor deletion macro. Less needed now that C++11 has
// introduced '= delete' on default class methods, but I still like to use it
// because I like how it looks.
#define DISALLOW_COPY_AND_ASSIGN(ClassName)  \
  ClassName(const ClassName&) = delete;      \
  ClassName& operator=(ClassName) = delete

namespace util {

// unique_ptr factory. Shim until C++14 is standardized.
template<typename T, typename ...Args>
std::unique_ptr<T> make_unique(Args&& ...args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}  // namespace util

#endif  // UTIL_H_
