#include "game/flocking.hh"
#include "core/hash.hh"
#include "core/random.hh"
#include "game/catalog.hh"
#include "game/component/boid.hh"
#include "game/component/figure.hh"
#include "game/component/pose.hh"
#include "game/component/velocity.hh"
#include "game/context.hh"
#include "game/entity.hh"
#include "game/flock.hh"
#include "game/registry.hh"
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

namespace { ////////////////////////////////////////////////////////////////////////////////

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
  [[nodiscard]] glm::vec2 steer_towards(glm::vec2 direction, glm::vec2 velocity, Flock const& flock)
  {
    f32 const length_squared = glm::dot(direction, direction);
    if (length_squared <= 0.f) return { 0.f, 0.f };
    glm::vec2 const desired = direction * (flock.max_speed / std::sqrt(length_squared));
    return clamp_length(desired - velocity, flock.max_force);
  }

} //////////////////////////////////////////////////////////////////////////////////////////

void spawn_flock(Context ctx, Token<Flock> flock, u32 count, u64 seed, Rectangle bounds)
{
  PRECONDITION(access_catalog(ctx).valid(flock), "spawned an undefined flock");
  PRECONDITION(bounds.area() > 0.f, "spawned a flock with nowhere to go");
  Flock const& definition = access_catalog(ctx)[flock];

  // Mixing the flock into the seed keeps two flocks spawned from the same world seed from being
  // laid out identically on top of each other.
  Xoshiro256ss rng{ split_mix(seed ^ hash_combine(0x801D5u, flock.value)) };

  for (u32 i = 0u; i < count; ++i) {
    glm::vec2 const position
      = bounds.min() + bounds.extent * glm::vec2{ unit_float(rng), unit_float(rng) };
    f32 const heading = unit_float(rng) * glm::two_pi<f32>();
    f32 const speed
      = definition.min_speed + (definition.max_speed - definition.min_speed) * unit_float(rng);

    auto boid = create_entity(ctx);
    boid.emplace<Pose>(position);
    boid.emplace<Velocity>(glm::vec2{ std::cos(heading), std::sin(heading) } * speed);
    boid.emplace<Boid>(flock);
    boid.emplace<Figure>(definition.sprite);
  }
}

void Flocking::step(Context ctx, f32 dt)
{
  PRECONDITION(dt > 0.f, "flocking needs a fixed, positive timestep");
  auto const catalog = access_catalog(ctx);
  count_             = 0u;
  for (u32 i = 0u, defined = catalog.count<Flock>(); i < defined; ++i) {
    Token<Flock> const token{ i };
    step_flock(ctx, token, catalog[token], dt);
  }
}

void Flocking::step_flock(Context ctx, Token<Flock> token, Flock const& flock, f32 dt)
{
  u32 const count = gather(ctx, token);
  if (count == 0u) return;
  count_ += count;

  // One cell per vision radius: a boid's neighbourhood then spans the nine cells around it, which
  // is the smallest window that cannot miss a flockmate in range.
  u32 const columns = u32(bounds_.width() / flock.vision) + 1u;
  u32 const rows    = u32(bounds_.height() / flock.vision) + 1u;
  index_.reset(bounds_.x(), bounds_.y(), flock.vision, columns, rows);
  index_.fill(count, [this](u32 i) { return positions_[i]; });

  steer(flock);
  integrate(ctx, flock, dt);
}

u32 Flocking::gather(Context ctx, Token<Flock> token)
{
  // TODO: replace the scan with a component query once the registry can join stores. Every flock
  // walks every live entity, which is affordable for a demo and not much else.
  auto const registry = access_registry(ctx);
  handles_.clear();
  positions_.clear();
  velocities_.clear();
  for (auto const& [handle, id] : registry) {
    Boid const* boid = registry.try_get<Boid>(handle);
    if (!boid || boid->flock != token) continue;
    Pose const*     pose     = registry.try_get<Pose>(handle);
    Velocity const* velocity = registry.try_get<Velocity>(handle);
    if (!pose || !velocity) continue; // a boid that cannot be placed or moved is not steered
    handles_.push_back(handle);
    positions_.push_back(pose->position);
    velocities_.push_back(velocity->linear);
  }
  INVARIANT(positions_.size() == handles_.size() && velocities_.size() == handles_.size());
  return u32(handles_.size());
}

void Flocking::steer(Flock const& flock)
{
  u32 const count = u32(positions_.size());
  steering_.assign(count, glm::vec2{ 0.f, 0.f });

  f32 const vision_squared = flock.vision * flock.vision;
  f32 const space_squared  = flock.personal_space * flock.personal_space;

  for (u32 i = 0u; i < count; ++i) {
    glm::vec2 const position = positions_[i];
    glm::vec2 const velocity = velocities_[i];

    glm::vec2 push{ 0.f, 0.f };    // away from flockmates that are too close
    glm::vec2 heading{ 0.f, 0.f }; // summed velocity of the neighbourhood
    glm::vec2 centre{ 0.f, 0.f };  // summed position of the neighbourhood
    u32       neighbours = 0u;

    index_.each_near(position.x, position.y, flock.vision, [&](u32 j) {
      if (j == i) return;
      glm::vec2 const offset           = positions_[j] - position;
      f32 const       distance_squared = glm::dot(offset, offset);
      if (distance_squared > vision_squared) return; // a candidate from an overlapping cell only
      heading += velocities_[j];
      centre  += positions_[j];
      ++neighbours;
      // Closer flockmates push harder: the inverse square falloff is what keeps a dense flock from
      // collapsing into a point without pushing apart one that is merely nearby.
      if (distance_squared < space_squared)
        push -= offset / std::max(distance_squared, min_distance_squared);
    });

    glm::vec2 urge{ 0.f, 0.f };
    if (neighbours > 0u) {
      f32 const share  = 1.f / f32(neighbours);
      urge            += steer_towards(centre * share - position, velocity, flock) * flock.cohesion;
      urge            += steer_towards(heading * share, velocity, flock) * flock.alignment;
    }
    urge += steer_towards(push, velocity, flock) * flock.separation;
    urge += containment(position, flock.margin) * (flock.containment * flock.max_force);

    steering_[i] = clamp_length(urge, flock.max_force);
  }
}

void Flocking::integrate(Context ctx, Flock const& flock, f32 dt)
{
  auto const registry = access_registry(ctx);
  u32 const  count    = u32(positions_.size());
  DEBUG_ASSERT(steering_.size() == count);

  for (u32 i = 0u; i < count; ++i) {
    glm::vec2 velocity = velocities_[i] + steering_[i] * dt;

    f32 const speed_squared = glm::dot(velocity, velocity);
    if (speed_squared > flock.max_speed * flock.max_speed)
      velocity *= flock.max_speed / std::sqrt(speed_squared);
    else if (speed_squared > 0.f && speed_squared < flock.min_speed * flock.min_speed)
      velocity *= flock.min_speed / std::sqrt(speed_squared);

    glm::vec2 position = positions_[i] + velocity * dt;

    // Containment is a steering urge, so it can be outrun: a boid banking hard at the edge may
    // still cross it. Rather than let one escape and drag the rest of the flock after it, put it
    // back on the boundary and turn the offending component of its flight around.
    glm::vec2 const lo = bounds_.min();
    glm::vec2 const hi = bounds_.max();
    if (position.x < lo.x || position.x > hi.x) {
      position.x = glm::clamp(position.x, lo.x, hi.x);
      velocity.x = -velocity.x;
    }
    if (position.y < lo.y || position.y > hi.y) {
      position.y = glm::clamp(position.y, lo.y, hi.y);
      velocity.y = -velocity.y;
    }

    Handle<Entity> const handle = handles_[i];
    DEBUG_ASSERT(registry.valid(handle), "boid died mid-step");
    registry.get<Pose>(handle).position   = position;
    registry.get<Velocity>(handle).linear = velocity;
  }
}

glm::vec2 Flocking::containment(glm::vec2 position, f32 margin) const noexcept
{
  DEBUG_ASSERT(margin > 0.f);
  glm::vec2 const lo = bounds_.min();
  glm::vec2 const hi = bounds_.max();
  glm::vec2       push{ 0.f, 0.f };
  if (position.x < lo.x + margin) push.x = (lo.x + margin - position.x) / margin;
  else if (position.x > hi.x - margin) push.x = (hi.x - margin - position.x) / margin;
  if (position.y < lo.y + margin) push.y = (lo.y + margin - position.y) / margin;
  else if (position.y > hi.y - margin) push.y = (hi.y - margin - position.y) / margin;
  return push;
}
