#include <stdint.h>
#include <stdio.h>
#include <memory>

#include <Eigen/Dense>
#include <SDL.h>
#include <boost/format.hpp>
#include <glog/logging.h>

#include "eigen-util.h"
#include "input.h"
#include "pieces.h"
#include "sdl-util.h"

using ::boost::format;
using ::eigen_util::FormatVec2d;
using ::sdl_util::ManagedWindow;
using ::sdl_util::SDLContext;

namespace pong {

constexpr int kDesiredFPS = 60;

class App {
 public:
  App(SDL_Window* window) : window_(CHECK_NOTNULL(window)) {}
  void Run();

 private:
  void ProcessEvents();
  void UpdateGame();
  void Render();

  bool running_;  // Whether the main loop is currently running
  PlayerInput input_;
  int last_update_millis_ {0};
  Ball player_;
  SDL_Window* window_;  // Not owned
};

void App::Run() {
  constexpr int kMillisPerFrame = 1000 / kDesiredFPS;

  running_ = true;
  while (running_) {
    int millis_before = SDL_GetTicks();

    ProcessEvents();
    UpdateGame();
    Render();

    int sleep_millis = kMillisPerFrame - (SDL_GetTicks() - millis_before);
    if (sleep_millis > 0) {
      SDL_Delay(sleep_millis);
    }
  }
}

void App::ProcessEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0) {
    if ((event.type == SDL_QUIT) ||
        (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_q)) {
      running_ = false;
    }
    input_.ProcessSdlKeyEvent(event);
  }
}

void App::UpdateGame() {
  int millis_now = SDL_GetTicks();
  if (last_update_millis_ != 0) {  // Don't update on first frame
    int millis_delta = millis_now - last_update_millis_;
    auto unit_direction = input_.MovementDirection();
    player_.Move(unit_direction, millis_delta);

    constexpr int kLogSeconds = 1;
    DLOG_EVERY_N(INFO, kDesiredFPS * kLogSeconds)
        << format("%s\ndirection%s\n%s") % player_
                                         % FormatVec2d(unit_direction)
                                         % input_.dpad;
  }
  last_update_millis_ = millis_now;
}

void App::Render() {
  SDL_Surface* screen_surface = SDL_GetWindowSurface(window_);
  // Clear screen w/ black color
  SDL_FillRect(screen_surface, nullptr,
               SDL_MapRGB(screen_surface->format, 0, 0, 0));

  SDL_Rect rect = player_.BoundingBox();
  SDL_FillRect(screen_surface, &rect,
               SDL_MapRGB(screen_surface->format, 0xFF, 0xFF, 0xFF));

  SDL_UpdateWindowSurface(window_);
}

}  // namespace pong

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);

  LOG(INFO) << "Initializing...";
  SDLContext sdl(SDL_INIT_VIDEO);
  CHECK(sdl.Success()) << "Could not initialize SDL: " << SDL_GetError();

  LOG(INFO) << "Creating window";
  constexpr int kScreenXPos = SDL_WINDOWPOS_UNDEFINED;
  constexpr int kScreenYPos = SDL_WINDOWPOS_UNDEFINED;
  constexpr int kScreenWidth  = 640;
  constexpr int kScreenHeight = 480;
  ManagedWindow window(SDL_CreateWindow("Hello World!", kScreenXPos,
        kScreenYPos, kScreenWidth, kScreenHeight, SDL_WINDOW_SHOWN));
  CHECK(window) << "Could not create SDL window: " << SDL_GetError();

  LOG(INFO) << "Starting main loop";
  pong::App app(window.get());
  app.Run();

  return 0;
}
