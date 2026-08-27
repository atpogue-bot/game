#pragma once

// TODO: draw order. Figures are currently drawn in registry order, which is stable but arbitrary.

#include "game/sprite.hh"

// What an entity looks like. Its presence declares that the entity is visible: the user interface
// draws every entity that has both a `Figure` and a `Pose`.
struct Figure
{
  Sprite sprite;
};
