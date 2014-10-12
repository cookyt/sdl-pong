#ifndef GAME_H_
#define GAME_H_

#include <ostream>

#include <Eigen/Dense>
#include <glog/logging.h>

#include "controller.h"

namespace pong {

enum class BoundingWall {
  NONE,
  TOP,
  BOTTOM,
  LEFT,
  RIGHT,
};
std::ostream& operator<<(std::ostream& stream, BoundingWall wall);

// Bounding box for 2d objects in game-units (gu). For posterity, I'll say one
// gu = one meter. Note that game units do not define a mapping to pixels on
// screen.
struct BoundingBox {
  BoundingBox() : top_left(0, 0), size(0, 0) {}
  BoundingBox(double left_x, double top_y, double width, double height)
      : top_left(left_x, top_y), size(width, height) {}

  // convenience method to control position based on bounding box center.
  Eigen::Vector2d Center() const { return top_left + (size * 0.5); }
  void Center(const Eigen::Vector2d new_val) {
    top_left = new_val - (size * 0.5);
  }

  // Convenience methods for getting the coordinates of the edges of the
  // bounding box.
  double Left() const { return top_left.x(); }
  double Right() const { return top_left.x() + size.x(); }
  double Top() const { return top_left.y(); }
  double Bottom() const { return top_left.y() + size.y(); }

  void Left(double new_val) { top_left.x() = new_val; }
  void Right(double new_val) { top_left.x() = new_val - size.x(); }
  void Top(double new_val) { top_left.y() = new_val; }
  void Bottom(double new_val) { top_left.y() = new_val - size.y(); }

  // Same as Left(), Right(), Top(), or Bottom(), but takes the wall to get a
  // bound for by parameter.
  double Bound(BoundingWall wall) const {
    switch (wall) {
      case BoundingWall::TOP:    return Top();
      case BoundingWall::BOTTOM: return Bottom();
      case BoundingWall::LEFT:   return Left();
      case BoundingWall::RIGHT:  return Right();
      default:
        LOG(FATAL) << "Tried to get bound for unsupported wall: "
                   << static_cast<int>(wall);
    }
  }

  // Convenience functions for the size of the box.
  double Width() const { return size.x(); }
  double Height() const { return size.y(); }

  void Width(double new_val) { size.x() = new_val; }
  void Height(double new_val) { size.y() = new_val; }

  friend std::ostream& operator<<(std::ostream& stream, const BoundingBox& box);

  Eigen::Vector2d top_left;
  Eigen::Vector2d size;  // width, height
};

// forward declarations for mutually-referenced classes
class GameBoard;

class Paddle {
 public:
  struct MoveParams {
  };

  Paddle(GameBoard* game_board)
      : game_board_(CHECK_NOTNULL(game_board)) {}

  void SetController(PaddleController* controller) { controller_ = controller; }

  void Update(double seconds_delta);

  // The "ceiling" beyond which the paddle can't pass upwards.
  double top_bound_;

  // The "floor" beyond which the paddle can't pass downwards.
  double bottom_bound_;

  // How fast the paddle moves. game units per second.
  double max_speed_;

  BoundingBox bounds_;

 private:
  GameBoard* game_board_;  // the containing game board. not owned.
  PaddleController* controller_ = nullptr;  // not owned
};

class Ball {
 public:
  Ball(GameBoard* game_board) : game_board_(CHECK_NOTNULL(game_board)) {}

  void Update(double seconds_delta);

  friend std::ostream& operator<<(std::ostream& stream, Ball ball);

  BoundingBox bounds_;
  Eigen::Vector2d velocity_;  // game units per second

  // Ball can only move freely within this box. If it's going to move out of
  // this box, it calls back to game_board->BounceBall to ask it for a new
  // direction.  Note that the ball must be strictly smaller than this space's
  // size.
  //
  // TODO: Intuitively, it feels like valid_space_ should be placed in the same
  // logical unit that defines bouncing characteristics. Spreading it between
  // this bounding box and the GameBoard feels wrong. Maybe make a seperate
  // object?
  BoundingBox valid_space_;

 private:
  GameBoard* game_board_;  // the containing game board. not owned.
};

class GameBoard {
 public:
  enum class Player { NONE, LEFT, RIGHT };

  // TODO have to deal w/ aspect ratio of game board vs aspect ratio of
  // rendering window. The square aspect ratio here isn't working well.
  GameBoard() : ball_(this), left_paddle_(this), right_paddle_(this) {
    SetupNewGame();
  }

  // Resets the game board without changing player scores. Puts the ball in the
  // the paddles in their initial positions (on their respective sides of the
  // board, and verticall in the center), and sets the ball's initial velocity
  // to serve in the direction of the right player.
  void SetupNewGame();
  bool IsGameOver() { return game_over_; }
  Player LastPlayerToScore() { return last_player_to_score_; }

  void Update(double seconds_delta);

  void BounceBall(Ball* ball, BoundingWall hit_wall);

  void SetLeftController(PaddleController* controller) {
    left_paddle_.SetController(controller);
  }
  void SetRightController(PaddleController* controller) {
    right_paddle_.SetController(controller);
  }

  // Game pieces
  Ball ball_;
  Paddle left_paddle_;
  Paddle right_paddle_;

  // Defines the area within which all game pieces (ball and both paddles)
  // should be visible when the game is running.
  BoundingBox bounds_ = {0, 0, 1, 1};

  int left_score_ = 0;
  int right_score_ = 0;

 private:
  bool game_over_ = true;
  Player last_player_to_score_ = Player::NONE;
};

std::ostream& operator<<(std::ostream& stream, GameBoard::Player player);

}  // namespace pong

#endif  // GAME_H_
