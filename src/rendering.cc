#include <Eigen/Dense>
#include <SDL_ttf.h>
#include <boost/format.hpp>

#include "game.h"
#include "rendering.h"

namespace pong {

namespace {
// converts a pong::BoundingBox to an SDL_Rect.
// `scale` defines the number of screen pixels per game unit.
// `origin` defines the on-screen pixel location of the game-world origin.
SDL_Rect BoundsToSdlRect(const BoundingBox& bounds,
                         const Eigen::Vector2d& origin,
                         const Eigen::Vector2d& scale) {
  return {
    static_cast<int>(bounds.Left() * scale.x() + origin.x()),
    static_cast<int>(bounds.Top() * scale.y() + origin.y()),
    static_cast<int>(bounds.Width() * scale.x()),
    static_cast<int>(bounds.Height() * scale.y()),
  };
}
}  // namespace


// NOTE on variable names: px := pixel(s), gu := game_unit(s)
void RenderGameToSdlSurface(const GameBoard& game, SDL_Surface* surface) {
  // Clear screen w/ black color
  SDL_FillRect(surface, nullptr,
               SDL_MapRGB(surface->format, 0, 0, 0));

  Eigen::Vector2d px_per_gu = {surface->w / game.bounds_.Width(),
                               surface->h / game.bounds_.Height()};
  Eigen::Vector2d origin_px = {game.bounds_.top_left.x() * px_per_gu.x(),
                               game.bounds_.top_left.y() * px_per_gu.y()};

  // Fill in a white rect for the ball
  SDL_Rect rect = BoundsToSdlRect(game.ball_.bounds_, origin_px, px_per_gu);
  SDL_FillRect(surface, &rect,
               SDL_MapRGB(surface->format, 0xFF, 0xFF, 0xFF));

  // Fill in a white rect for the left paddle
  rect =
      BoundsToSdlRect(game.left_player_.paddle.bounds_, origin_px, px_per_gu);
  SDL_FillRect(surface, &rect,
               SDL_MapRGB(surface->format, 0xFF, 0xFF, 0xFF));

  // Fill in a white rect for the right paddle
  rect =
      BoundsToSdlRect(game.right_player_.paddle.bounds_, origin_px, px_per_gu);
  SDL_FillRect(surface, &rect,
               SDL_MapRGB(surface->format, 0xFF, 0xFF, 0xFF));

  // White line in the middle.
  int line_width_px =
      (game.ball_.bounds_.Width() / 2) * px_per_gu.x();
  int board_center_x = static_cast<int>(origin_px.x()) + surface->w / 2;
  SDL_Rect middle_line_rect = {
    board_center_x - (line_width_px / 2),
    0,
    line_width_px,
    surface->h,
  };
  SDL_FillRect(surface, &middle_line_rect,
               SDL_MapRGB(surface->format, 0xFF, 0xFF, 0xFF));
}

}  // namespace pong
