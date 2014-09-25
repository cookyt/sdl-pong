#include <stdint.h>
#include <stdio.h>
#include <memory>

#include <Eigen/Dense>
#include <SDL.h>
#include <glog/logging.h>

#include "sdl-util.h"
#include "pieces.h"

using ::sdl_util::SDLContext;
using ::sdl_util::ManagedWindow;

namespace pong {

constexpr int kDesiredFPS = 60;
constexpr int kMillisPerFrame = 1000 / kDesiredFPS;

// A virtual "controller" which describes the state of player input at a given
// moment in time.
struct PlayerInput {
  direction::Vertical vertical {direction::Vertical::NONE};
  direction::Horizontal horizontal {direction::Horizontal::NONE};
};

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
  int vert_acc = static_cast<int>(input_.vertical);
  int horz_acc = static_cast<int>(input_.horizontal);
  while (SDL_PollEvent(&event) != 0) {
    if (event.type == SDL_QUIT) {
      running_ = false;
    }
    if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
        case SDLK_UP:
          if (vert_acc != -1) vert_acc += -1;
          break;
        case SDLK_DOWN:
          if (vert_acc != 1) vert_acc += 1;
          break;
        case SDLK_RIGHT:
          if (horz_acc != 1) horz_acc += 1;
          break;
        case SDLK_LEFT:
          if (horz_acc != -1) horz_acc += -1;
          break;
      }
    } else if (event.type == SDL_KEYUP) {
      switch (event.key.keysym.sym) {
        case SDLK_UP:
          if (vert_acc != 1) vert_acc += 1;
          break;
        case SDLK_DOWN:
          if (vert_acc != -1) vert_acc += -1;
          break;
        case SDLK_RIGHT:
          if (horz_acc != -1) horz_acc += -1;
          break;
        case SDLK_LEFT:
          if (horz_acc != 1) horz_acc += 1;
          break;
      }
    }
  }
  // Apply the net changes of the processed events to the state of the input
  // controller.
  DCHECK(vert_acc >= -1 && vert_acc <= 1);
  DCHECK(horz_acc >= -1 && horz_acc <= 1);
  input_.vertical = static_cast<direction::Vertical>(vert_acc);
  input_.horizontal = static_cast<direction::Horizontal>(horz_acc);
}

void App::UpdateGame() {
  DLOG_EVERY_N(INFO, kDesiredFPS * 2) << player_;  // approx. 2 seconds
  int millis_now = SDL_GetTicks();
  if (last_update_millis_ != 0) {  // Don't update on first frame
    int millis_delta = millis_now - last_update_millis_;
    player_.Move(input_.vertical, input_.horizontal, millis_delta);
  }
  last_update_millis_ = millis_now;
}

void App::Render() {
  SDL_Surface* screen_surface = SDL_GetWindowSurface(window_);
  // Clear screen w/ black color
  SDL_FillRect(screen_surface, nullptr,
               SDL_MapRGB(screen_surface->format, 0, 0, 0));

  SDL_Rect rect = player_.AsRect();
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
