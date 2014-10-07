#include <glog/logging.h>

#include "input.h"

using ::pong::DPad;

namespace pong {

std::ostream& operator<<(std::ostream& out, const DPad& dpad) {
  using boost::format;
  return out << format("U:%d D:%d L:%d R:%d") % (dpad.IsSet<DPad::UP>())
                                              % (dpad.IsSet<DPad::DOWN>())
                                              % (dpad.IsSet<DPad::LEFT>())
                                              % (dpad.IsSet<DPad::RIGHT>());
}

int DPad::GetCanceled() const {
  return
    ((IsSet<UP>()    && !IsSet<DOWN>())  ? UP    : 0) |
    ((IsSet<DOWN>()  && !IsSet<UP>())    ? DOWN  : 0) |
    ((IsSet<LEFT>()  && !IsSet<RIGHT>()) ? LEFT  : 0) |
    ((IsSet<RIGHT>() && !IsSet<LEFT>())  ? RIGHT : 0);
}

void PlayerInput::ProcessSdlKeyEvent(const SDL_Event& event) {
  if (event.type == SDL_KEYDOWN) {
    switch (event.key.keysym.sym) {
      case SDLK_UP:    dpad.Set<DPad::UP>();    break;
      case SDLK_DOWN:  dpad.Set<DPad::DOWN>();  break;
      case SDLK_LEFT:  dpad.Set<DPad::LEFT>();  break;
      case SDLK_RIGHT: dpad.Set<DPad::RIGHT>(); break;
    }
  } else if (event.type == SDL_KEYUP) {
    switch (event.key.keysym.sym) {
      case SDLK_UP:    dpad.UnSet<DPad::UP>();    break;
      case SDLK_DOWN:  dpad.UnSet<DPad::DOWN>();  break;
      case SDLK_LEFT:  dpad.UnSet<DPad::LEFT>();  break;
      case SDLK_RIGHT: dpad.UnSet<DPad::RIGHT>(); break;
    }
  }
}

Eigen::Vector2d PlayerInput::MovementDirection() const {
  const double kInvRoot2 = sqrt(0.5);
  int direction = dpad.GetCanceled();
  switch (direction) {
    // 4 cardinal directions
    case DPad::UP:    return {0, -1};
    case DPad::DOWN:  return {0, 1};
    case DPad::LEFT:  return {-1, 0};
    case DPad::RIGHT: return {1, 0};

    // 4 diagonal directions
    case DPad::LEFT  | DPad::UP:   return {-kInvRoot2, -kInvRoot2};
    case DPad::LEFT  | DPad::DOWN: return {-kInvRoot2, kInvRoot2};
    case DPad::RIGHT | DPad::UP:   return {kInvRoot2,  -kInvRoot2};
    case DPad::RIGHT | DPad::DOWN: return {kInvRoot2,  kInvRoot2};

    // No buttons held down currently
    case 0: return {0, 0};
  }
  LOG(FATAL) << "got unexpected button combination the dpad - not a valid "
                "8-directional value: " << direction;
}


}  // namespace pong
