#ifndef PIECES_H_
#define PIECES_H_

#include <ostream>
#include <string>

#include <Eigen/Dense>
#include <SDL.h>

using std::string;

namespace pong {

// TODO convert to using directional vector2d scheme to avoid annoying switches
// everywhere.
namespace direction {
enum class Vertical {
  UP = -1,
  NONE = 0,
  DOWN = 1,
};
enum class Horizontal {
  LEFT = -1,
  NONE = 0,
  RIGHT = 1,
};
}  // namespace direction

class Ball {
 public:
  const double kAcceleration = 50;
  const double kMaxVel = 50;
  const double kFrictionAccel = 3;

  SDL_Rect BoundingBox() const;

  // Applies friction, acceleration, and velocity to the ball. See Accelerate()
  // for parameter definitions.
  void Move(const Eigen::Vector2d& direction, int millis_delta);

  friend std::ostream& operator<<(std::ostream& out, const Ball& ball);

 private:
  // Slows down a moving ball.
  // seconds_delta: number of seconds the ball is moving over.
  void ApplyFriction(double seconds_delta);

  // Moves the position of the ball as per its current velocity.
  // seconds_delta: number of seconds the ball is moving over.
  void ApplyVelocity(double seconds_delta);

  // Accelerates the ball using its internal acceleration constant.
  // vert: What direction to accelerate in the y axis
  // horz: what direction to accelerate in the x axis
  // seconds_delta: number of seconds the ball is moving over.
  void Accelerate(const Eigen::Vector2d& unit_direction, double seconds_delta);

  Eigen::Vector2d vel_ {0, 0};
  Eigen::Vector2d pos_ {0, 0};
  Eigen::Vector2d size_ {32, 32};  // width, height
};

}  // pong

#endif  // PIECES_H_
