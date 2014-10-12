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

  double time_to_wall = std::abs((wall_pos - bound) / vel);

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
  auto min_time_to_wall = MinTimeToWall(*this);
  int num_times_bounced = 0;
  while (std::get<0>(min_time_to_wall) < seconds_delta) {
    DLOG(INFO)
        << boost::format(
               "bouncing ball=%s wall=%s min_time_to_wall=%s time_delta=%s") %
               *this % std::get<1>(min_time_to_wall) %
               std::get<0>(min_time_to_wall) % seconds_delta;

    // Spend some time to move the ball to the point of contact with the wall
    bounds_.top_left += std::get<0>(min_time_to_wall) * velocity_;
    seconds_delta -= std::get<0>(min_time_to_wall);

    // Let the game board handle to bounce
    game_board_->BounceBall(this, std::get<1>(min_time_to_wall));

    // The previous bouce of the ball caused the game to end (one of the players
    // missed the return), so don't bother trying to recalculate ball position.
    // TODO: responding to the end of the game this way feels a bit janky.
    if (!game_board_->IsRunning()) {
      break;
    }

    // Recalculate whether the ball will bounce off another wall in the same
    // update's time frame.
    min_time_to_wall = MinTimeToWall(*this);

    // TODO: in a confined space, this might loop forever. I don't have any out
    // in that case, but it shouldn't matter for a normal game's setup.
    num_times_bounced += 1;
  }

  if (num_times_bounced != 0) {
    DLOG(INFO) << "ball bounced " << num_times_bounced
               << " times in one update";
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
  left_player_.paddle.bounds_.Width(kBallSize_gu);
  left_player_.paddle.bounds_.Height(3 * kBallSize_gu);
  left_player_.paddle.bounds_.Center(bounds_.Center());
  left_player_.paddle.bounds_.Left(bounds_.Left());
  left_player_.paddle.top_bound_ = bounds_.Top();
  left_player_.paddle.bottom_bound_ = bounds_.Bottom();
  left_player_.paddle.max_speed_ = kPaddleSpeed_gups;

  // setup right paddle
  right_player_.paddle.bounds_.Width(kBallSize_gu);
  right_player_.paddle.bounds_.Height(3 * kBallSize_gu);
  right_player_.paddle.bounds_.Center(bounds_.Center());
  right_player_.paddle.bounds_.Right(bounds_.Right());
  right_player_.paddle.top_bound_ = bounds_.Top();
  right_player_.paddle.bottom_bound_ = bounds_.Bottom();
  right_player_.paddle.max_speed_ = kPaddleSpeed_gups;

  // setup ball
  // TODO: randomized who the ball is served to, and at what angle.
  ball_.bounds_.size = {kBallSize_gu, kBallSize_gu};
  ball_.bounds_.Center(bounds_.Center());
  ball_.velocity_ = util::DirectionAndMagnitude(
      {1, 2}, kInitialBallSpeed_gups);  // arbitrary

  ball_.valid_space_.top_left = {left_player_.paddle.bounds_.Right(),
                                 bounds_.Top()};
  ball_.valid_space_.Width(right_player_.paddle.bounds_.Left() -
                           left_player_.paddle.bounds_.Right());
  ball_.valid_space_.Height(bounds_.Height());
}

void GameBoard::Update(double seconds_delta) {
  if (running_) {
    left_player_.paddle.Update(seconds_delta);
    right_player_.paddle.Update(seconds_delta);
    ball_.Update(seconds_delta);
  }
}

namespace {
// Returns true if the ball will bounce off the paddle in this update. Just
// checks the Y positions and Heights of both objects, ignores the X position
// and width.
bool WillBounce(const Paddle& paddle, const Ball& ball) {
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
  game->left_player_.paddle.max_speed_ *= kPaddleSpeedupFactor;
  game->right_player_.paddle.max_speed_ *= kPaddleSpeedupFactor;
}
}  // namespace

void GameBoard::BounceBall(Ball* ball, BoundingWall hit_wall) {
  switch (hit_wall) {
    case BoundingWall::TOP:  // fallthrough
    case BoundingWall::BOTTOM:
      ball->velocity_.y() *= -1;
      break;

    case BoundingWall::LEFT:
      if (WillBounce(left_player_.paddle, ball_)) {
        BounceBallOffPaddle(this);
      } else {
        right_player_.score += 1;
        running_ = false;
      }
      break;

    case BoundingWall::RIGHT:
      if (WillBounce(right_player_.paddle, ball_)) {
        BounceBallOffPaddle(this);
      } else {
        left_player_.score += 1;
        running_ = false;
      }
      break;

    default:
      LOG(FATAL) << "Unexpected value for wall off which the ball is bouncing: "
                 << static_cast<int>(hit_wall);
  }
}

}  // namespace pong
