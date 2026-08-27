#pragma once

// TODO: predators. A flock should be able to name the things its members flee from.

#include "game/sprite.hh"
#include "game/types.hh"

struct lua_State;

// One kind of flock: how its members look, and the weights that balance the three classic boid
// urges (separation, alignment, cohesion) against each other and against staying in bounds.
//
// Distances are world units, speeds units per second, and forces units per second squared. The
// weights are relative to one another and have no unit; only their ratios matter.
struct Flock
{
  Sprite sprite;         // how members of this flock are drawn
  f32    vision;         // radius within which a member perceives its flockmates
  f32    personal_space; // radius within which a member pushes flockmates away
  f32    min_speed;      // members never fly slower than this: a bird that stops falls
  f32    max_speed;      // ... and never faster than this
  f32    max_force;      // the hardest a member can turn
  f32    separation;     // weight of the urge to keep out of a flockmate's personal space
  f32    alignment;      // weight of the urge to match the heading of nearby flockmates
  f32    cohesion;       // weight of the urge to close on the centre of nearby flockmates
  f32    containment;    // weight of the urge to turn back inside the flock's bounds
  f32    margin;         // distance from the bounds at which containment starts to pull
};

namespace lua { void add_flock_builder(lua_State* L, CatalogWriter& catalog); }
