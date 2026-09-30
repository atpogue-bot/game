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
#include "game/area.hh"
#include "game/entity.hh"
#include <glm/vec2.hpp>
#include <vector>

namespace Game {
  struct Flock;
  struct World;

  // Create [count] members of [flock], scattered over [area] and already in flight. Reproducible:
  // the same seed and arguments always produce the same birds in the same places.
  void spawn_flock(World& world, Handle<Flock> flock, u32 count, u64 seed, Area area);

  // The flocking system. Holds the per-tick working set so that steering a flock allocates once
  // rather than every tick, which is why it is an object rather than a free function.
  struct Flocking
  {
    explicit Flocking(Area bounds) noexcept : bounds_{ bounds } {}

    Flocking(Flocking&&) noexcept            = default;
    Flocking(Flocking const&)                = delete;
    Flocking& operator=(Flocking&&) noexcept = default;
    Flocking& operator=(Flocking const&)     = delete;
    ~Flocking() noexcept                     = default;

    // Steer and move every boid one tick. [dt] must be the fixed simulation timestep: the flock is
    // reproducible for a fixed dt and only for a fixed dt.
    void step(World& world, f32 dt);

    // The area boids are kept inside.
    [[nodiscard]] Area bounds() const noexcept { return bounds_; }

    // How many boids were steered by the last `step`.
    [[nodiscard]] u32 count() const noexcept { return count_; }

  private:

    // Collect every boid in the world into the working set, and every flock they belong to into
    // `kinds_`. Returns how many boids were found.
    u32 gather(World const& world);

    // Steer the members of one flock. Flocks are handled one at a time because the neighbourhood
    // index is sized from the flock's own vision radius, and because boids of different flocks
    // ignore each other.
    void step_flock(World& world, Handle<Flock> kind, Flock const& flock, f32 dt);

    // Fill `members_` with the indices of the gathered boids belonging to [kind].
    void select(Handle<Flock> kind);

    // Fill `steering_` with the force acting on each selected boid this tick.
    void steer(Flock const& flock);

    // Apply `steering_`, then write the result back onto the entities it came from.
    void integrate(World& world, Flock const& flock, f32 dt);

    // The force pulling a boid back inside `bounds_`, ramping from nothing at [margin] to full
    // strength at the edge. Magnitude is per-axis and in [0, 1] inside the bounds.
    [[nodiscard]] glm::vec2 containment(glm::vec2 position, f32 margin) const noexcept;

    Area bounds_;
    u32  count_ = 0u;

    // Every boid in the world, in registry order: parallel arrays gathered once per tick so that
    // steering reads positions and velocities without walking back through the registry.
    std::vector<Handle<Entity>> handles_;
    std::vector<glm::vec2>      positions_;
    std::vector<glm::vec2>      velocities_;
    std::vector<Handle<Flock>>  membership_;

    // The distinct flocks found by the last gather, in the order they were first seen.
    std::vector<Handle<Flock>> kinds_;

    // Indices into the arrays above naming the members of the flock being steered, and the force
    // on each of them, parallel to one another.
    SpatialGrid            index_;
    std::vector<u32>       members_;
    std::vector<glm::vec2> steering_;
  };
}
