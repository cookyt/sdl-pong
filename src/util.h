// This file defines some miscellaneous utilities to be used throughout code.

#ifndef UTIL_H_
#define UTIL_H_

#include <memory>
#include <ostream>
#include <utility>

#include <Eigen/Dense>
#include <SDL.h>
#include <SDL_ttf.h>
#include <boost/format.hpp>
#include <glog/logging.h>

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

// Constructs a Vec2d from a direction vector and a magnitude. Note that the
// magnitude of the passed-in `direction` is unimportant (as long is it's
// non-zero).
inline Eigen::Vector2d DirectionAndMagnitude(const Eigen::Vector2d& direction,
                                             double magnitude) {
  return direction.normalized() * magnitude;
}


namespace format {
inline boost::format FormatVec2d(const Eigen::Vector2d& vec) {
  return boost::format("{%lf %lf}") % vec.x() % vec.y();
}

inline boost::format FormatSdlRect(const SDL_Rect& rect) {
  return boost::format("{x:%s y:%s w:%s h:%s}") % rect.x % rect.y % rect.w
                                                % rect.h;
}
}  // namespace format


namespace sdl {
// Deleter for use in std::unqiue_ptr to call SDL_DestroyWindow automatically.
struct WindowDeleter {
  void operator()(SDL_Window* window) { SDL_DestroyWindow(window); }
};

struct FontDeleter {
  void operator()(TTF_Font* font) { TTF_CloseFont(font); }
};

struct SurfaceDeleter {
  void operator()(SDL_Surface* surface) { SDL_FreeSurface(surface); }
};

// Smart pointers to objects created through SDL functions.
typedef std::unique_ptr<SDL_Window, WindowDeleter> ManagedWindow;
typedef std::unique_ptr<TTF_Font, FontDeleter> ManagedFont;
typedef std::unique_ptr<SDL_Surface, SurfaceDeleter> ManagedSurface;

// Simple class that calls SDL_Init when instantiated and calls SDL_Quit when
// destroyed.
class SDLContext {
 public:
  explicit SDLContext(int flags) : status_(SDL_Init(flags)) {}
  ~SDLContext() { if (Success()) { SDL_Quit(); } }
  bool Success() { return status_ == 0; }
  void CheckSuccess() {
    CHECK(Success()) << "Could not initialize SDL: " << SDL_GetError();
  }

  const int status_;

  DISALLOW_COPY_AND_ASSIGN(SDLContext);
};

// Simple class that calls TTF_Init when instantiated and calls TTF_Quit when
// destroyed.
class TTFContext {
 public:
  TTFContext() : status_(TTF_Init()) {}
  ~TTFContext() { if (Success()) { TTF_Quit(); } }
  bool Success() { return status_ == 0; }
  void CheckSuccess() {
    CHECK(Success()) << "Could not initialize SDL_TTF: " << TTF_GetError();
  }

  const int status_;

  DISALLOW_COPY_AND_ASSIGN(TTFContext);
};
}  // namespace sdl

}  // namespace util

#endif  // UTIL_H_
