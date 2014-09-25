// Simple utilities for managing SDL resources using RAII.

#ifndef SDH_UTIL_H_
#define SDH_UTIL_H_

#include <memory>

#include <SDL.h>

#define DISALLOW_COPY_AND_ASSIGN(ClassName)  \
  ClassName(const ClassName&) = delete;      \
  ClassName& operator=(ClassName) = delete;

namespace sdl_util {

// Deleter for use in std::unqiue_ptr to call SDL_DestroyWindow automatically.
struct SDLWindowDeleter {
  void operator()(SDL_Window* window) { SDL_DestroyWindow(window); }
};

// A Managed SDL Window pointer that destroys the window when this object is
// destroyed. Meant to be used in tandem w/ SDL_CreateWindow.
typedef std::unique_ptr<SDL_Window, SDLWindowDeleter> ManagedWindow;

// Simple class that calls SDL_Init when instantiated and calls SDL_Quit when
// destroyed.
class SDLContext {
 public:
  explicit SDLContext(int flags) : status_(SDL_Init(flags)) {}
  ~SDLContext() { if (Success()) { SDL_Quit(); } }
  inline bool Success() { return status_ == 0; }

  const int status_;

  DISALLOW_COPY_AND_ASSIGN(SDLContext);
};

}  // namespace sdl_util

#endif  // SDH_UTIL_H_
