#ifndef RENDERING_H_
#define RENDERING_H_

#include <SDL.h>

namespace pong {

class GameBoard;

// Renders a pong::GameBoard to a given SDL surface. It fills simple white
// rectangles on a black surface to give a look and feel similar to classic
// pong. It considers game.bounds_ to be the entire visible area of the board,
// and stretches it to fit in the surface.
//
// TODO add a way to preserve aspect ratio w/ letter-boxing.
void RenderGameToSdlSurface(const GameBoard& game, SDL_Surface *surface);

}  // namespace pong

#endif  // RENDERING_H_
