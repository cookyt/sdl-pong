#include <cmath>
#include <limits>
#include <boost/format.hpp>
#include <glog/logging.h>

#include "controller.h"
#include "game.h"
#include "util.h"

namespace pong {

std::ostream& operator<<(std::ostream& stream, BoundingWall wall) {
  switch (wall) {
    case BoundingWall::NONE:   return (stream << "NONE");
    case BoundingWall::TOP:    return (stream << "TOP");
    case BoundingWall::BOTTOM: return (stream << "BOTTOM");
    case BoundingWall::LEFT:   return (stream << "LEFT");
    case BoundingWall::RIGHT:  return (stream << "RIGHT");
    default:
      LOG(WARNING) << "Tried to serialize unexpected pong::BoundingWall value: "
                   << static_cast<int>(wall);
      return stream << static_cast<int>(wall);
  }
}

std::ostream& operator<<(std::ostream& stream, const BoundingBox& box) {
  using util::format::FormatVec2d;
  return stream << boost::format("BoundingBox(top_left=%s, size=%s)") %
                       FormatVec2d(box.top_left) % FormatVec2d(box.size);
}

void Paddle::Update(double seconds_delta) {
  if (controller_ == nullptr) {  // null controllers have no effect
    return;
  }

  // Apply the movement in the desired direction of the controller.
  double delta_position = seconds_delta * max_speed_;
  MoveDirection direction = controller_->DesiredMove(*game_board_, *this);
  if (direction == MoveDirection::UP) {
    bounds_.top_left.y() -= delta_position;
  } else if (direction == MoveDirection::DOWN) {
    bounds_.top_left.y() += delta_position;
  } else {
    DCHECK(direction == MoveDirection::NONE)
        << "unexpected result from DesiredMove: "
        << static_cast<int>(direction);
  }

  // Clamp the paddle to the top and bottom bounds.
  if (bounds_.Top() < top_bound_) {
    bounds_.top_left.y() = top_bound_;
  }
  if (bounds_.Bottom() > bottom_bound_) {
    bounds_.top_left.y() = bottom_bound_ - bounds_.Height();
  }
}


namespace {
using ::std::tuple;

tuple<double, BoundingWall> MinTimeToWall(const Ball& ball, double vel,
                                          BoundingWall negative_wall,
                                          BoundingWall positive_wall) {
  if (vel == 0) {
    return std::make_tuple(std::numeric_limits<double>::infinity(),
                           BoundingWall::NONE);
  }

  BoundingWall wall;
  double wall_pos;
  double bound;

  if (vel < 0) {
    wall = negative_wall;
    wall_pos = ball.valid_space_.Bound(negative_wall);
    bound = ball.bounds_.Bound(negative_wall);
  } else {
    wall = positive_wall;
    wall_pos = ball.valid_space_.Bound(positive_wall);
    bound = ball.bounds_.Bound(positive_wall);
  }

  double time_to_wall = (wall_pos - bound) / vel;
  CHECK(time_to_wall >= 0) << "Unexpected negative time_to_wall for a Ball";
  return std::make_tuple(time_to_wall, wall);
}

tuple<double, BoundingWall> MinTimeToWall(const Ball& ball) {
  auto time_to_x_wall =
      MinTimeToWall(ball, ball.velocity_.x(), BoundingWall::LEFT,
                    BoundingWall::RIGHT);

  auto time_to_y_wall =
      MinTimeToWall(ball, ball.velocity_.y(), BoundingWall::TOP,
                    BoundingWall::BOTTOM);

  auto min_time =
      (time_to_x_wall < time_to_y_wall) ? time_to_x_wall : time_to_y_wall;

  return min_time;
}
}  // namespace

void Ball::Update(double seconds_delta) {
  // TODO: in a confined space, this might loop forever. I don't have any out in
  // that case, but it shouldn't matter for a normal game's setup.
  auto min_time_to_wall = MinTimeToWall(*this);
  while (std::get<0>(min_time_to_wall) < seconds_delta) {
    // Spend some time to move the ball to the point of contact with the wall
    bounds_.top_left += std::get<0>(min_time_to_wall) * velocity_;
    seconds_delta -= std::get<0>(min_time_to_wall);

    // Let the game board handle to bounce
    game_board_->BounceBall(this, std::get<1>(min_time_to_wall));

    // The previous bouce of the ball caused the game to end (one of the players
    // missed the return), so don't bother trying to recalculate ball position.
    // TODO: responding to the end of the game this way feels a bit janky.
    if (game_board_->IsGameOver()) {
      break;
    }

    // Recalculate whether the ball will bounce off another wall in the same
    // update's time frame.
    min_time_to_wall = MinTimeToWall(*this);
  }

  bounds_.top_left += seconds_delta * velocity_;
}

std::ostream& operator<<(std::ostream& stream, Ball ball) {
  using ::util::format::FormatVec2d;
  return stream << boost::format("Ball(bounds=%s velocity=%s)") %
                       ball.bounds_ % FormatVec2d(ball.velocity_);
}


namespace {
constexpr double kBallSize_gu = 0.05;
constexpr double kInitialBallSpeed_gups = 0.2;
constexpr double kPaddleSpeed_gups = kBallSize_gu * 10;
}  // namespace

void GameBoard::SetupNewGame() {
  // setup left paddle
  left_paddle_.bounds_.Width(kBallSize_gu);
  left_paddle_.bounds_.Height(3 * kBallSize_gu);
  left_paddle_.bounds_.Center(bounds_.Center());
  left_paddle_.bounds_.Left(bounds_.Left());
  left_paddle_.top_bound_ = bounds_.Top();
  left_paddle_.bottom_bound_ = bounds_.Bottom();
  left_paddle_.max_speed_ = kPaddleSpeed_gups;

  // setup right paddle
  right_paddle_.bounds_.Width(kBallSize_gu);
  right_paddle_.bounds_.Height(3 * kBallSize_gu);
  right_paddle_.bounds_.Center(bounds_.Center());
  right_paddle_.bounds_.Right(bounds_.Right());
  right_paddle_.top_bound_ = bounds_.Top();
  right_paddle_.bottom_bound_ = bounds_.Bottom();
  right_paddle_.max_speed_ = kPaddleSpeed_gups;

  // setup ball
  // TODO: randomized who the ball is served to, and at what angle.
  ball_.bounds_.size = {kBallSize_gu, kBallSize_gu};
  ball_.bounds_.Center(bounds_.Center());
  ball_.velocity_ = util::DirectionAndMagnitude(
      {1, 2}, kInitialBallSpeed_gups);  // arbitrary

  ball_.valid_space_.top_left = {left_paddle_.bounds_.Right(),
                                 bounds_.Top()};
  ball_.valid_space_.Width(right_paddle_.bounds_.Left() -
                           left_paddle_.bounds_.Right());
  ball_.valid_space_.Height(bounds_.Height());

  game_over_ = false;
}

void GameBoard::Update(double seconds_delta) {
  if (!IsGameOver()) {
    left_paddle_.Update(seconds_delta);
    right_paddle_.Update(seconds_delta);
    ball_.Update(seconds_delta);
  }
}

namespace {
// Returns true if the ball will bounce off the paddle in this update. Just
// checks the Y positions and Heights of both objects, ignores the X position
// and width.
bool WillBounce(const Ball& ball, const Paddle& paddle) {
  if (ball.bounds_.Top() > paddle.bounds_.Bottom() ||
      ball.bounds_.Bottom() < paddle.bounds_.Top()) {
    return false;
  }
  return true;
}

// Every time the ball is successfully bounced, the speed of the ball and the
// paddles increases by this factor.
constexpr double kBallSpeedupFactor = 1.1;
constexpr double kPaddleSpeedupFactor = 1.05;
inline void BounceBallOffPaddle(GameBoard* game) {
  game->ball_.velocity_.x() *= -1;
  game->ball_.velocity_ *= kBallSpeedupFactor;
  game->left_paddle_.max_speed_ *= kPaddleSpeedupFactor;
  game->right_paddle_.max_speed_ *= kPaddleSpeedupFactor;
}
}  // namespace

void GameBoard::BounceBall(Ball* ball, BoundingWall hit_wall) {
  switch (hit_wall) {
    case BoundingWall::TOP:  // fallthrough
    case BoundingWall::BOTTOM:
      ball->velocity_.y() *= -1;
      break;

    case BoundingWall::LEFT:
      if (WillBounce(ball_, left_paddle_)) {
        BounceBallOffPaddle(this);
      } else {
        right_score_ += 1;
        last_player_to_score_ = Player::RIGHT;
        game_over_ = true;
      }
      break;

    case BoundingWall::RIGHT:
      if (WillBounce(ball_, right_paddle_)) {
        BounceBallOffPaddle(this);
      } else {
        left_score_ += 1;
        last_player_to_score_ = Player::LEFT;
        game_over_ = true;
      }
      break;

    default:
      LOG(FATAL) << "Unexpected value for wall off which the ball is bouncing: "
                 << static_cast<int>(hit_wall);
  }
}

std::ostream& operator<<(std::ostream& stream, GameBoard::Player player) {
  switch (player) {
    case GameBoard::Player::NONE:  return stream << "NONE";
    case GameBoard::Player::LEFT:  return stream << "LEFT";
    case GameBoard::Player::RIGHT: return stream << "RIGHT";
    default:
      LOG(WARNING)
          << "Tried to serialize unexpected pong::GameBoard::Player value: "
          << static_cast<int>(player);
      return stream << static_cast<int>(player);
  }
}

}  // namespace pong
