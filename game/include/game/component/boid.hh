#pragma once
#include "core/types.hh"

struct Flock;

// Membership of a flock. Its presence declares that the entity is steered by the flocking system,
// and names the rules it obeys. A boid only perceives members of its own flock, so several flocks
// can share the same space without merging into one.
struct Boid
{
  Token<Flock> flock;
};
