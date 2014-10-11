#include <stdint.h>
#include <stdio.h>
#include <memory>

#include <Eigen/Dense>
#include <SDL.h>
#include <SDL_ttf.h>
#include <boost/algorithm/string/join.hpp>
#include <boost/format.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include "controller.h"
#include "game.h"
#include "rendering.h"
#include "util.h"

DEFINE_string(data_path, "data",
              "The directory in which to look for data files.");

using ::boost::format;
using ::util::format::FormatSdlRect;
using ::util::format::FormatVec2d;
using ::util::sdl::ManagedWindow;
using ::util::sdl::SDLContext;
using ::util::sdl::TTFContext;

namespace pong {

constexpr int kDesiredFPS = 60;

class App {
 public:
  App(SDL_Window* window);
  void Run();

 private:
  void ProcessEvents();
  void UpdateGame();
  void Render();

  bool running_;  // Whether the main loop is currently running
  int last_update_millis_ {0};
  SdlPaddleController left_controller_;
  FollowBallYController right_controller_;
  GameBoard game_;
  SDL_Window* window_;  // Not owned
};

App::App(SDL_Window* window) : window_(CHECK_NOTNULL(window)) {
  game_.SetLeftController(&left_controller_);
  game_.SetRightController(&right_controller_);
}

void App::Run() {
  constexpr int kMillisPerFrame = 1000 / kDesiredFPS;

  running_ = true;
  game_.Start();
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
    left_controller_.ProcessSdlEvent(event);
  }
}

void App::UpdateGame() {
  int millis_now = SDL_GetTicks();
  if (last_update_millis_ != 0) {  // Don't update on first frame
    double millis_delta = millis_now - last_update_millis_;
    bool ran_before_update = game_.IsRunning();
    game_.Update(millis_delta / 1000.0);
    if (ran_before_update && !game_.IsRunning()) {
      // TODO add mechanism to determince who scored.
      LOG(INFO) << "Player scored! New score: left:" << game_.left_player_.score
                << " right:" << game_.right_player_.score;
    }
  }
  last_update_millis_ = millis_now;
}

void App::Render() {
  SDL_Surface* screen_surface = SDL_GetWindowSurface(window_);
  RenderGameToSdlSurface(game_, screen_surface);
  SDL_UpdateWindowSurface(window_);
}

}  // namespace pong

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  LOG(INFO) << "Initializing SDL";
  SDLContext sdl(SDL_INIT_VIDEO);
  sdl.CheckSuccess();

  LOG(INFO) << "Initializing SDL_TTF";
  TTFContext ttf;
  ttf.CheckSuccess();

  const SDL_Rect kScreenParams = {
    SDL_WINDOWPOS_UNDEFINED,  // x
    SDL_WINDOWPOS_UNDEFINED,  // y
    640,                      // width
    640,                      // height
  };
  LOG(INFO) << "Creating SDL window with params: "
            << FormatSdlRect(kScreenParams);
  ManagedWindow window(SDL_CreateWindow("Hello World!", kScreenParams.x,
                                        kScreenParams.y, kScreenParams.w,
                                        kScreenParams.h, SDL_WINDOW_SHOWN));
  CHECK(window) << "Could not create SDL window: " << SDL_GetError();

  LOG(INFO) << "Starting main loop";
  pong::App app(window.get());
  app.Run();

  return 0;
}
