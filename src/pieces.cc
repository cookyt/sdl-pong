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
  Eigen::Vector2d top_left = pos_ - (size_ / 2.);
  return {
      static_cast<int>(top_left.x()),
      static_cast<int>(top_left.y()),
      static_cast<int>(size_[0]),
      static_cast<int>(size_[1]),
  };
}

void Ball::Move(const Eigen::Vector2d& direction, int millis_delta) {
  double seconds_delta = (static_cast<double>(millis_delta) / 1000.);

  // Only apply friction if not being accelerated. This avoid the friction
  // acceleration and the explicit acceleration fighting.
  ApplyFriction(seconds_delta);
  if (!direction.isZero()) {
    Accelerate(direction.normalized(), kAcceleration, seconds_delta);
  }

  ApplyVelocity(seconds_delta);
}

std::ostream& operator<<(std::ostream& out, const Ball& ball) {
  return out << format("pos:(%lf, %lf); vel:(%lf, %lf)") % ball.pos_.x()
                                                         % ball.pos_.y()
                                                         % ball.vel_.x()
                                                         % ball.vel_.y();
}

void Ball::ApplyFriction(double seconds_delta) {
  if (vel_.isZero())
    return;

  double frict_accel_norm = kFrictionAccel * seconds_delta;
  if (frict_accel_norm > vel_.norm()) {
    vel_ = {0, 0};
  } else {
    vel_ -= (vel_.normalized() * frict_accel_norm);
  }
}

void Ball::ApplyVelocity(double seconds_delta) {
  pos_ += (seconds_delta * vel_);
}

void Ball::Accelerate(const Eigen::Vector2d& unit_direction,
                      double accel_m_per_sec2, double seconds_delta) {
  double delta_accel = seconds_delta * accel_m_per_sec2;
  vel_ += (unit_direction * delta_accel);
  if (vel_.norm() > kMaxVel) {
    vel_.normalize();
    vel_ *= kMaxVel;
  }
}

}  // namespace pong
