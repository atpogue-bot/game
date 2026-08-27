#pragma once

// TODO: angular velocity, once `Pose` carries a rotation.

#include <glm/vec2.hpp>

// How fast an entity is travelling and in what direction. Its presence declares that the entity
// moves continuously and is integrated by a simulation system, rather than being displaced a step
// at a time by an action.
struct Velocity
{
  glm::vec2 linear; // world units per second
};
