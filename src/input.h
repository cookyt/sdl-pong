#ifndef INPUT_H_
#define INPUT_H_

#include <ostream>

#include <Eigen/Dense>
#include <SDL.h>
#include <boost/format.hpp>

namespace pong {

// Structure representing a 4 directional button-based input where each button
// can be independently pressed or depressed.
class DPad {
 public:
  // Bitflags corresponding to potential buttons being down.
  enum Direction {
    UP    = 1 << 0,
    DOWN  = 1 << 1,
    LEFT  = 1 << 2,
    RIGHT = 1 << 3,
  };

  // Returns whether the given button(s) is(are) pressed down.
  template<Direction Dir>
  bool IsSet() const {
    AssertValidButtonCombo<Dir>();
    return (bits_ & Dir) != 0;
  }

  // Presses and holds the button(s) corresponding to Dir.
  template<Direction Dir>
  void Set() {
    AssertValidButtonCombo<Dir>();
    bits_ |= Dir;
  }

  // Releases the button(s) corresponding to Dir
  template<Direction Dir>
  void UnSet() {
    AssertValidButtonCombo<Dir>();
    bits_ &= ~Dir;
  }

  // Returns the raw bitset with Direction bitflags set for each button
  // currently down.
  int Get() const { return bits_; }

  // Returns a version of this DPad with conflicting directions cancelled out.
  // That is, if both UP and DOWN are on in the DPad, then the returned value
  // will have neither set (ditto for LEFT and RIGHT).
  int GetCanceled() const;

  friend std::ostream& operator<<(std::ostream& out, const DPad& dpad);

 private:
  // Wrapper for a static_assert which assures that you're setting flags
  // correctly.
  template<Direction Dir>
  void AssertValidButtonCombo() const {
    static_assert(
        (Dir & (UP | DOWN | LEFT | RIGHT)) &&      // At least one valid bit set
            !(Dir & ~(UP | DOWN | LEFT | RIGHT)),  // No invalid bits set
        "invalid direction flag or combination thereof");
  }

  int bits_ = 0;
};

// A virtual "controller" which describes the state of player input at a given
// moment in time.
struct PlayerInput {
  // What directional keys are being pressed.
  DPad dpad;

  // Returns a unit vector representing the direction the player wants to move
  // in on a 2D field. Basically converts the 8-directional dpad to one of 8
  // vectors. Axis is oriented with positive directions being down and to the
  // right.
  Eigen::Vector2d MovementDirection() const;

  // If the given event is a keydown or keyup, and the keysym is one of
  // SDLK_{UP,DOWN,LEFT,RIGHT}, then the corresponding button on this dpad is
  // set or unset.
  void ProcessSdlKeyEvent(const SDL_Event& event);
};

}  // namespace pong

#endif  // INPUT_H_
