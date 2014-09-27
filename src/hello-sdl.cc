#include <stdint.h>
#include <stdio.h>
#include <memory>

#include <Eigen/Dense>
#include <SDL.h>
#include <boost/format.hpp>
#include <glog/logging.h>

#include "eigen-util.h"
#include "pieces.h"
#include "sdl-util.h"

using ::boost::format;
using ::eigen_util::FormatVec2d;
using ::sdl_util::ManagedWindow;
using ::sdl_util::SDLContext;

namespace pong {

constexpr int kDesiredFPS = 60;
constexpr int kMillisPerFrame = 1000 / kDesiredFPS;


// Structure representing a 4 directional button-based input where each button
// can be independently pressed or depressed.
class DPad {
 public:
  // Bitflags corresponding to potential buttons being down.
  // See `int AsBits()'
  enum BitFlags {
    UP    = 1 << 0,
    DOWN  = 1 << 1,
    LEFT  = 1 << 2,
    RIGHT = 1 << 3,
  };

  template<int Dir>
  bool IsSet() const {
    AssertValidButtonCombo<Dir>();
    return static_cast<bool>((bits_ & Dir));
  }

  template<int Dir>
  void Set() {
    AssertValidButtonCombo<Dir>();
    bits_ |= Dir;
  }

  template<int Dir>
  void UnSet() {
    AssertValidButtonCombo<Dir>();
    bits_ &= ~Dir;
  }

  int Get() const { return bits_; }

  // Returns a version of this DPad with conflicting directions cancelled out.
  // That is, if both UP and DOWN are on in the DPad, then the returned value
  // will have neither set (ditto for LEFT and RIGHT).
  int GetCanceled() const {
    return
      ((IsSet<UP>()    && !IsSet<DOWN>())  ? UP    : 0) |
      ((IsSet<DOWN>()  && !IsSet<UP>())    ? DOWN  : 0) |
      ((IsSet<LEFT>()  && !IsSet<RIGHT>()) ? LEFT  : 0) |
      ((IsSet<RIGHT>() && !IsSet<LEFT>())  ? RIGHT : 0);
  }

  friend std::ostream& operator<<(std::ostream& out, const DPad& dpad) {
    return out << format("U:%d D:%d L:%d R:%d") %
               (dpad.IsSet<DPad::UP>()) %
               (dpad.IsSet<DPad::DOWN>()) %
               (dpad.IsSet<DPad::LEFT>()) %
               (dpad.IsSet<DPad::RIGHT>());
  }

 private:
  // Wrapper for the static_assert.
  template<int Dir>
  void AssertValidButtonCombo() const {
    static_assert(
        (Dir & (UP | DOWN | LEFT | RIGHT)) &&      // At least one valid bit set
            !(Dir & ~(UP | DOWN | LEFT | RIGHT)),  // No invalid bits set
        "invalid direction flag or combination thereof");
  }

  template <int Dir>
  const char* DirectionName() {
    switch(Dir) {
      case UP:
        return "U";
      case DOWN:
        return "D";
      case LEFT:
        return "L";
      case RIGHT:
        return "R";
      default:
        return "";
    }
  }

  int bits_ = 0;
};


// A virtual "controller" which describes the state of player input at a given
// moment in time.
struct PlayerInput {
  // What directional keys are being pressed.
  DPad dpad;

  // Returns a unit vector representing the direction the player wants to move
  // in on a 2D field. Axis is oriented with positive directions being down and
  // to the right 8-directional.
  Eigen::Vector2d MovementDirection();
};

Eigen::Vector2d PlayerInput::MovementDirection() {
  const double kInvRoot2 = sqrt(0.5);
  int direction = dpad.GetCanceled();
  switch (direction) {
    case DPad::UP:
      return {0, -1};
    case DPad::DOWN:
      return {0, 1};
    case DPad::LEFT:
      return {-1, 0};
    case DPad::RIGHT:
      return {1, 0};
    case DPad::LEFT | DPad::UP:
      return {-kInvRoot2, -kInvRoot2};
    case DPad::LEFT | DPad::DOWN:
      return {-kInvRoot2, kInvRoot2};
    case DPad::RIGHT | DPad::UP:
      return {kInvRoot2, -kInvRoot2};
    case DPad::RIGHT | DPad::DOWN:
      return {kInvRoot2, kInvRoot2};
    case 0:
      return {0, 0};
  }
  LOG(FATAL) << "got unexpected button combination the dpad - not a valid "
                "8-directional value: " << direction;
}

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
  while (SDL_PollEvent(&event) != 0) {
    if (event.type == SDL_QUIT) {
      running_ = false;
    }
    if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
        case SDLK_UP:
          input_.dpad.Set<DPad::UP>();
          break;
        case SDLK_DOWN:
          input_.dpad.Set<DPad::DOWN>();
          break;
        case SDLK_LEFT:
          input_.dpad.Set<DPad::LEFT>();
          break;
        case SDLK_RIGHT:
          input_.dpad.Set<DPad::RIGHT>();
          break;
      }
    } else if (event.type == SDL_KEYUP) {
      switch (event.key.keysym.sym) {
        case SDLK_UP:
          input_.dpad.UnSet<DPad::UP>();
          break;
        case SDLK_DOWN:
          input_.dpad.UnSet<DPad::DOWN>();
          break;
        case SDLK_LEFT:
          input_.dpad.UnSet<DPad::LEFT>();
          break;
        case SDLK_RIGHT:
          input_.dpad.UnSet<DPad::RIGHT>();
          break;
      }
    }
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
