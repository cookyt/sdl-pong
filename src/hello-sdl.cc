#include <stdio.h>
#include <memory>

#include <SDL.h>
#include <glog/logging.h>

#include "sdl-util.h"

using ::sdl_util::SDLContext;
using ::sdl_util::ManagedWindow;

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);

  LOG(INFO) << "Initializing...";
  SDLContext sdl(SDL_INIT_VIDEO);
  CHECK(sdl.Success()) << "Could not initialize SDL: " << SDL_GetError();

  LOG(INFO) << "Creating window";
  constexpr int kScreenWidth  = 640;
  constexpr int kScreenHeight = 480;
  ManagedWindow window(SDL_CreateWindow("Hello World!", SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED, kScreenWidth,
                                        kScreenHeight, SDL_WINDOW_SHOWN));
  CHECK(window) << "Could not create SDL window: " << SDL_GetError();

  LOG(INFO) << "Coloring window white";
  SDL_Surface* screen_surface = SDL_GetWindowSurface(window.get());
  SDL_FillRect(screen_surface, nullptr,
               SDL_MapRGB(screen_surface->format, 0xFF, 0xFF, 0xFF));
  SDL_UpdateWindowSurface(window.get());
  SDL_Delay(2000);

  return 0;
}
