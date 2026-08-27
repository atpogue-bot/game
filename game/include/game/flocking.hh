#pragma once

// Reynolds' boids. Every member of a flock steers by three local rules -- keep your distance, fly
// the way your neighbours fly, stay with the group -- plus a fourth that turns it back before it
// leaves the simulated area. Nothing coordinates the flock: the shape it takes is what those four
// urges add up to.
//
// TODO: express the steering as a command so that a flock is directed like any other actor. It is
// a system today because `Command` carries a single unit step and a batch is capped well below a
// flock's population.

#include "core/spatial-grid.hh"
#include "game/types.hh"
#include "gfx/rectangle.hh"
#include <glm/vec2.hpp>
#include <vector>

struct Flock;

// Create [count] members of [flock], scattered over [bounds] and already in flight. Reproducible:
// the same seed and arguments always produce the same birds in the same places.
void spawn_flock(Context ctx, Token<Flock> flock, u32 count, u64 seed, Rectangle bounds);

// The flocking system. Holds the per-tick working set so that steering a flock allocates once
// rather than every tick, which is why it is an object rather than a free function.
struct Flocking
{
  explicit Flocking(Rectangle bounds) noexcept : bounds_{ bounds } {}

  Flocking(Flocking&&) noexcept            = default;
  Flocking(Flocking const&)                = delete;
  Flocking& operator=(Flocking&&) noexcept = default;
  Flocking& operator=(Flocking const&)     = delete;
  ~Flocking() noexcept                     = default;

  // Steer and move every boid one tick. [dt] must be the fixed simulation timestep: the flock is
  // reproducible for a fixed dt and only for a fixed dt.
  void step(Context ctx, f32 dt);

  // The area boids are kept inside.
  [[nodiscard]] Rectangle bounds() const noexcept { return bounds_; }

  // How many boids were steered by the last `step`.
  [[nodiscard]] u32 count() const noexcept { return count_; }

private:

  // Steer the members of one flock. Flocks are handled one at a time because the neighbourhood
  // index is sized from the flock's own vision radius, and because boids of different flocks
  // ignore each other.
  void step_flock(Context ctx, Token<Flock> token, Flock const& flock, f32 dt);

  // Collect the members of [token] into the working set. Returns how many were found.
  u32 gather(Context ctx, Token<Flock> token);

  // Fill `steering_` with the force acting on each gathered boid this tick.
  void steer(Flock const& flock);

  // Apply `steering_`, then write the result back onto the entities it came from.
  void integrate(Context ctx, Flock const& flock, f32 dt);

  // The force pulling a boid back inside `bounds_`, ramping from nothing at [margin] to full
  // strength at the edge. Magnitude is per-axis and in [0, 1] inside the bounds.
  [[nodiscard]] glm::vec2 containment(glm::vec2 position, f32 margin) const noexcept;

  Rectangle bounds_;
  u32       count_ = 0u;

  // The working set: parallel arrays, one entry per boid of the flock being stepped.
  SpatialGrid                 index_;
  std::vector<Handle<Entity>> handles_;
  std::vector<glm::vec2>      positions_;
  std::vector<glm::vec2>      velocities_;
  std::vector<glm::vec2>      steering_;
};
