#pragma once
#include "core/basic-catalog.hh"
#include "core/basic-registry.hh"
#include "game/chunk.hh"
#include "game/component/boid.hh"
#include "game/component/pose.hh"
#include "game/component/velocity.hh"
#include "game/content/flock.hh"
#include "game/content/terrain.hh"
#include "game/entity.hh"
#include <unordered_map>

namespace Game {
  // Append only: a definition's position in this list is its store.
  using Definitions = TypeList<Terrain, Flock>;
  using Catalog     = BasicCatalog<Definitions>;

  // Append only: a component's position in this list is its index into the registry's stores, so
  // inserting one in the middle renumbers every component after it.
  using Components = TypeList<Pose, Velocity, Boid>;
  using Registry   = BasicRegistry<Entity, Components>;

  struct World
  {
    Catalog  content;     // immutable after loading
    Registry entities;    // entity data
    Chunk    environment; // placeholder for a chunked world

    [[nodiscard]] Handle<Entity> find(Entity e) const;

    [[nodiscard]] Handle<Entity> create();

    void advance();

  private:

    // Total number of entities including those not loaded.
    u64 entity_counter_ = 0u;

    // Hash map used because entity IDs are sparse, not dense.
    std::unordered_map<Entity, Handle<Entity>> lookup_;
  };
}
