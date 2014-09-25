#include <math.h>

#include <SDL.h>
#include <boost/format.hpp>
#include <glog/logging.h>

#include "pieces.h"

using boost::format;

namespace pong {

SDL_Rect Ball::AsRect() {
  // TODO: width and height are arbitrary atm.
  return {
      static_cast<int>(pos_.x()),
      static_cast<int>(pos_.y()),
      static_cast<int>(size_[0]),
      static_cast<int>(size_[1]),
  };
}

void Ball::Move(direction::Vertical vert, direction::Horizontal horz,
                int millis_delta) {
  double seconds_delta = (static_cast<double>(millis_delta) / 1000.);
  ApplyFriction(seconds_delta);
  Accelerate(vert, horz, seconds_delta);
  ApplyVelocity(seconds_delta);
}

std::ostream& operator<<(std::ostream& out, const Ball& ball) {
  return out << format("pos:(%lf, %lf); vel:(%lf, %lf)") % ball.pos_.x()
                                                         % ball.pos_.y()
                                                         % ball.vel_.x()
                                                         % ball.vel_.y();
}

void Ball::ApplyFriction(double seconds_delta) {
  // TODO implement
}

void Ball::ApplyVelocity(double seconds_delta) {
  pos_ += (seconds_delta * vel_);
}

namespace {
// Modifies the vector `v' so that each component is within the range
// [-clamp_to, +clamp_to]
void Clamp(Eigen::Vector2d* v, const Eigen::Vector2d& clamp_to) {
  if (v->y() < -clamp_to.y()) {
    v->y() = -clamp_to.y();
  } else if (v->y() > clamp_to.y()) {
    v->y() = clamp_to.y();
  }

  if (v->x() < -clamp_to.x()) {
    v->x() = -clamp_to.x();
  } else if (v->x() > clamp_to.x()) {
    v->x() = clamp_to.x();
  }
}
}  // namespace

void Ball::Accelerate(direction::Vertical vert, direction::Horizontal horz,
                      double seconds_delta) {
  auto delta_accel =  seconds_delta * kAccel;
  switch (vert) {
    case direction::Vertical::UP:
      vel_.y() -= delta_accel.y();
      break;
    case direction::Vertical::DOWN:
      vel_.y() += delta_accel.y();
      break;
    default:
      break;  // Do Nothing
  }
  switch (horz) {
    case direction::Horizontal::LEFT:
      vel_.x() -= delta_accel.x();
      break;
    case direction::Horizontal::RIGHT:
      vel_.x() += delta_accel.x();
      break;
    default:
      break;  // Do Nothing
  }

  // Clamp velocity
  Clamp(&vel_, kMaxVel);
}

}  // namespace pong
