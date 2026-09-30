#pragma once
#include <glm/vec2.hpp>

namespace Game {
  // An axis-aligned area of the world. The graphics layer has `Rectangle`, but a simulation bound
  // is not a thing to draw, and `game` does not depend on `gfx`.
  struct Area
  {
    glm::vec2 min, max;

    [[nodiscard]] constexpr glm::vec2 extent() const noexcept { return max - min; }
  };
}
