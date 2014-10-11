#include <math.h>

#include "controller.h"
#include "game.h"

namespace pong {

std::ostream& operator<<(std::ostream& stream, MoveDirection dir) {
  switch (dir) {
    case MoveDirection::NONE: return (stream << "NONE");
    case MoveDirection::UP:   return (stream << "UP");
    case MoveDirection::DOWN: return (stream << "DOWN");
    default:
      LOG(WARNING)
          << "Tried to serialize unexpected pong::MoveDirection value: "
          << static_cast<int>(dir);
      return stream << static_cast<int>(dir);
  }
}

MoveDirection SdlPaddleController::DesiredMove(const GameBoard& game,
                                               const Paddle& paddle) {
  if (up_pressed_ && !down_pressed_) {
    return MoveDirection::UP;
  } else if (down_pressed_ && !up_pressed_) {
    return MoveDirection::DOWN;
  }
  return MoveDirection::NONE;
}

void SdlPaddleController::ProcessSdlEvent(const SDL_Event& event) {
  if (event.type == SDL_KEYDOWN) {
    if (event.key.keysym.sym == up_key_) {
      up_pressed_ = true;
    } else if (event.key.keysym.sym == down_key_) {
      down_pressed_ = true;
    }
  } else if (event.type == SDL_KEYUP) {
    if (event.key.keysym.sym == up_key_) {
      up_pressed_ = false;
    } else if (event.key.keysym.sym == down_key_) {
      down_pressed_ = false;
    }
  }
}

MoveDirection FollowBallYController::DesiredMove(const GameBoard& game,
                                                 const Paddle& paddle) {
  double start_move_tolerance = paddle.bounds_.Height() / 2;
  double stop_move_tolerance = paddle.bounds_.Height() / 4;
  // double tolerance = 0;
  double delta_y =
      paddle.bounds_.Center().y() - game.ball_.bounds_.Center().y();
  if ((!moving_ && std::abs(delta_y) > start_move_tolerance) ||
      (moving_ && std::abs(delta_y) > stop_move_tolerance)) {
    moving_ = true;
    if (delta_y < 0) {
      return MoveDirection::DOWN;
    } else {
      return MoveDirection::UP;
    }
  }
  moving_ = false;
  return MoveDirection::NONE;
}

}  // namespace pong
