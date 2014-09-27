#include <math.h>

#include <SDL.h>
#include <boost/format.hpp>
#include <glog/logging.h>

#include "eigen-util.h"
#include "pieces.h"

using boost::format;

namespace pong {

SDL_Rect Ball::BoundingBox() const {
  // TODO: width and height are arbitrary atm.
  return {
      static_cast<int>(pos_.x()),
      static_cast<int>(pos_.y()),
      static_cast<int>(size_[0]),
      static_cast<int>(size_[1]),
  };
}

void Ball::Move(const Eigen::Vector2d& direction, int millis_delta) {
  double seconds_delta = (static_cast<double>(millis_delta) / 1000.);

  Eigen::Vector2d unit_direction;
  if (direction.isZero()) {
    unit_direction = direction;
  } else {
    unit_direction = direction.normalized();
  }

  ApplyFriction(seconds_delta);
  Accelerate(unit_direction, seconds_delta);
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

void Ball::Accelerate(const Eigen::Vector2d& unit_direction,
                      double seconds_delta) {
  double delta_accel = seconds_delta * kAcceleration;
  vel_ += (unit_direction * delta_accel);
  if (vel_.norm() > kMaxVel) {
    vel_.normalize();
    vel_ *= kMaxVel;
  }
}

}  // namespace pong
