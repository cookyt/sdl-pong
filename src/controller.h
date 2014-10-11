#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include <ostream>

#include <SDL.h>

namespace pong {

class Paddle;
class GameBoard;

enum class MoveDirection {
  NONE,
  UP,
  DOWN,
};
std::ostream& operator<<(std::ostream& stream, MoveDirection dir);

class PaddleController {
 public:
  virtual ~PaddleController() {}

  // Implementors should override this to provide input to the paddle on how
  // to move.
  virtual MoveDirection DesiredMove(const GameBoard& game,
                                    const Paddle& paddle) {
    return MoveDirection::NONE;
  }
};

// This controller takes input from SDL keypress events. It's meant to allow a
// human player to control a paddle.
class SdlPaddleController : public PaddleController {
 public:
  SdlPaddleController(SDL_Keycode up_key = SDLK_UP,
                      SDL_Keycode down_key = SDLK_DOWN)
      : up_key_(up_key), down_key_(down_key) {}

  MoveDirection DesiredMove(const GameBoard& game,
                            const Paddle& paddle) override;

  // TODO maybe overenginnering, but adding an abstract base class
  // SdlEventProcessor so as to polymorphically handle event processing might be
  // nice.
  void ProcessSdlEvent(const SDL_Event& event);

 private:
  SDL_Keycode up_key_;
  SDL_Keycode down_key_;

  bool up_pressed_ = false;
  bool down_pressed_ = false;
};

// This controller represents a simple AI which always tries to keep the center
// of the ball aligned with the center of its paddle.
class FollowBallYController : public PaddleController {
 public:
  MoveDirection DesiredMove(const GameBoard& game,
                            const Paddle& paddle) override;
 private:
  bool moving_ = false;
};

// TODO Add a controller which allows for net play

}  // namespace pong

#endif  // CONTROLLER_H_
