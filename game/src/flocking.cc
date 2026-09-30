#include "game/flocking.hh"
#include "core/hash.hh"
#include "core/panic.hh"
#include "core/random.hh"
#include "game/component/boid.hh"
#include "game/component/pose.hh"
#include "game/component/velocity.hh"
#include "game/content/flock.hh"
#include "game/world.hh"
#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

namespace Game {
  namespace {
    // Two boids never sit exactly on top of one another for long, but they can come arbitrarily
    // close, and separation divides by the distance between them. Flooring it trades an unbounded
    // force for a merely very large one.
    constexpr f32 min_distance_squared = 1e-4f;

    // A uniform f32 in [0, 1), taken from the top bits of the generator by hand rather than with
    // `std::uniform_real_distribution`, whose mapping is implementation defined and would make a
    // flock reproducible only within one standard library.
    [[nodiscard]] f32 unit_float(Xoshiro256ss& rng) { return f32(rng() >> 40) * 0x1p-24f; }

    // Shorten [vector] to [limit] if it is longer, leaving its direction alone.
    [[nodiscard]] glm::vec2 clamp_length(glm::vec2 vector, f32 limit)
    {
      DEBUG_ASSERT(limit >= 0.f);
      f32 const length_squared = glm::dot(vector, vector);
      if (length_squared <= limit * limit) return vector;
      return vector * (limit / std::sqrt(length_squared));
    }

    // Reynolds' steering: the correction that turns the boid's current velocity into a full speed
    // run along [direction], capped by how hard the boid can actually turn.
    [[nodiscard]] glm::vec2
    steer_towards(glm::vec2 direction, glm::vec2 velocity, Flock const& flock)
    {
      f32 const length_squared = glm::dot(direction, direction);
      if (length_squared <= 0.f) return { 0.f, 0.f };
      glm::vec2 const desired = direction * (flock.max_speed / std::sqrt(length_squared));
      return clamp_length(desired - velocity, flock.max_force);
    }
  }

  void spawn_flock(World& world, Handle<Flock> flock, u32 count, u64 seed, Area area)
  {
    PRECONDITION(world.content.valid(flock), "spawned an undefined flock");
    PRECONDITION(
      area.extent().x > 0.f && area.extent().y > 0.f, "spawned a flock with nowhere to go");
    Flock const& definition = world.content[flock];

    // Mixing the flock into the seed keeps two flocks spawned from the same world seed from being
    // laid out identically on top of each other.
    Xoshiro256ss rng{ split_mix(seed ^ hash_combine(0x801D5u, flock.index)) };

    for (u32 i = 0u; i < count; ++i) {
      glm::vec2 const position
        = area.min + area.extent() * glm::vec2{ unit_float(rng), unit_float(rng) };
      f32 const heading = unit_float(rng) * glm::two_pi<f32>();
      f32 const speed
        = definition.min_speed + (definition.max_speed - definition.min_speed) * unit_float(rng);

      Handle<Entity> const handle = world.create();
      world.entities.emplace<Pose>(handle, position);
      world.entities.emplace<Velocity>(
        handle, glm::vec2{ std::cos(heading), std::sin(heading) } * speed);
      world.entities.emplace<Boid>(handle, flock);
    }
  }

  void Flocking::step(World& world, f32 dt)
  {
    PRECONDITION(dt > 0.f, "flocking needs a fixed, positive timestep");
    count_ = gather(world);
    if (count_ == 0u) return;
    for (Handle<Flock> const kind : kinds_) {
      step_flock(world, kind, world.content[kind], dt);
    }
  }

  u32 Flocking::gather(World const& world)
  {
    // TODO: replace the scan with a component query once the registry can join stores. Every tick
    // walks every live entity, which is affordable for a demo and not much else.
    handles_.clear();
    positions_.clear();
    velocities_.clear();
    membership_.clear();
    kinds_.clear();
    for (auto const& [handle, id] : world.entities) {
      Boid const* boid = world.entities.try_get<Boid>(handle);
      if (!boid || !world.content.valid(boid->flock)) continue;
      Pose const*     pose     = world.entities.try_get<Pose>(handle);
      Velocity const* velocity = world.entities.try_get<Velocity>(handle);
      if (!pose || !velocity) continue; // a boid that cannot be placed or moved is not steered
      handles_.push_back(handle);
      positions_.push_back(pose->position);
      velocities_.push_back(velocity->linear);
      membership_.push_back(boid->flock);
      if (std::ranges::find(kinds_, boid->flock) == kinds_.end()) kinds_.push_back(boid->flock);
    }
    INVARIANT(positions_.size() == handles_.size() && velocities_.size() == handles_.size());
    INVARIANT(membership_.size() == handles_.size());
    return u32(handles_.size());
  }

  void Flocking::step_flock(World& world, Handle<Flock> kind, Flock const& flock, f32 dt)
  {
    select(kind);
    u32 const count = u32(members_.size());
    if (count == 0u) return;

    // One cell per vision radius: a boid's neighbourhood then spans the nine cells around it,
    // which is the smallest window that cannot miss a flockmate in range.
    glm::vec2 const extent  = bounds_.extent();
    u32 const       columns = u32(extent.x / flock.vision) + 1u;
    u32 const       rows    = u32(extent.y / flock.vision) + 1u;
    index_.reset(bounds_.min.x, bounds_.min.y, flock.vision, columns, rows);
    index_.fill(count, [this](u32 i) { return positions_[members_[i]]; });

    steer(flock);
    integrate(world, flock, dt);
  }

  void Flocking::select(Handle<Flock> kind)
  {
    members_.clear();
    for (u32 i = 0u, gathered = u32(membership_.size()); i < gathered; ++i) {
      if (membership_[i] == kind) members_.push_back(i);
    }
  }

  void Flocking::steer(Flock const& flock)
  {
    u32 const count = u32(members_.size());
    steering_.assign(count, glm::vec2{ 0.f, 0.f });

    f32 const vision_squared = flock.vision * flock.vision;
    f32 const space_squared  = flock.personal_space * flock.personal_space;

    for (u32 i = 0u; i < count; ++i) {
      u32 const       self     = members_[i];
      glm::vec2 const position = positions_[self];
      glm::vec2 const velocity = velocities_[self];

      glm::vec2 push{ 0.f, 0.f };    // away from flockmates that are too close
      glm::vec2 heading{ 0.f, 0.f }; // summed velocity of the neighbourhood
      glm::vec2 centre{ 0.f, 0.f };  // summed position of the neighbourhood
      u32       neighbours = 0u;

      index_.each_near(position.x, position.y, flock.vision, [&](u32 j) {
        if (j == i) return;
        u32 const       other            = members_[j];
        glm::vec2 const offset           = positions_[other] - position;
        f32 const       distance_squared = glm::dot(offset, offset);
        if (distance_squared > vision_squared) return; // a candidate from an overlapping cell only
        heading += velocities_[other];
        centre  += positions_[other];
        ++neighbours;
        // Closer flockmates push harder: the inverse square falloff is what keeps a dense flock
        // from collapsing into a point without pushing apart one that is merely nearby.
        if (distance_squared < space_squared) {
          push -= offset / std::max(distance_squared, min_distance_squared);
        }
      });

      glm::vec2 urge{ 0.f, 0.f };
      if (neighbours > 0u) {
        f32 const share = 1.f / f32(neighbours);
        urge += steer_towards(centre * share - position, velocity, flock) * flock.cohesion;
        urge += steer_towards(heading * share, velocity, flock) * flock.alignment;
      }
      urge += steer_towards(push, velocity, flock) * flock.separation;
      urge += containment(position, flock.margin) * (flock.containment * flock.max_force);

      steering_[i] = clamp_length(urge, flock.max_force);
    }
  }

  void Flocking::integrate(World& world, Flock const& flock, f32 dt)
  {
    u32 const count = u32(members_.size());
    DEBUG_ASSERT(steering_.size() == count);

    for (u32 i = 0u; i < count; ++i) {
      u32 const self     = members_[i];
      glm::vec2 velocity = velocities_[self] + steering_[i] * dt;

      f32 const speed_squared = glm::dot(velocity, velocity);
      if (speed_squared > flock.max_speed * flock.max_speed) {
        velocity *= flock.max_speed / std::sqrt(speed_squared);
      } else if (speed_squared > 0.f && speed_squared < flock.min_speed * flock.min_speed) {
        velocity *= flock.min_speed / std::sqrt(speed_squared);
      }

      glm::vec2 position = positions_[self] + velocity * dt;

      // Containment is a steering urge, so it can be outrun: a boid banking hard at the edge may
      // still cross it. Rather than let one escape and drag the rest of the flock after it, put it
      // back on the boundary and turn the offending component of its flight around.
      if (position.x < bounds_.min.x || position.x > bounds_.max.x) {
        position.x = glm::clamp(position.x, bounds_.min.x, bounds_.max.x);
        velocity.x = -velocity.x;
      }
      if (position.y < bounds_.min.y || position.y > bounds_.max.y) {
        position.y = glm::clamp(position.y, bounds_.min.y, bounds_.max.y);
        velocity.y = -velocity.y;
      }

      Handle<Entity> const handle = handles_[self];
      DEBUG_ASSERT(world.entities.valid(handle), "boid died mid-step");
      world.entities.get<Pose>(handle).position   = position;
      world.entities.get<Velocity>(handle).linear = velocity;
    }
  }

  glm::vec2 Flocking::containment(glm::vec2 position, f32 margin) const noexcept
  {
    DEBUG_ASSERT(margin > 0.f);
    glm::vec2 const lo = bounds_.min;
    glm::vec2 const hi = bounds_.max;
    glm::vec2       push{ 0.f, 0.f };
    if (position.x < lo.x + margin) push.x = (lo.x + margin - position.x) / margin;
    else if (position.x > hi.x - margin) push.x = (hi.x - margin - position.x) / margin;
    if (position.y < lo.y + margin) push.y = (lo.y + margin - position.y) / margin;
    else if (position.y > hi.y - margin) push.y = (hi.y - margin - position.y) / margin;
    return push;
  }
}
