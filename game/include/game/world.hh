#pragma once
#include "component/pose.hh"
#include "content/terrain.hh"
#include "core/basic-catalog.hh"
#include "core/basic-registry.hh"
#include "game/chunk.hh"
#include "game/entity.hh"
#include <unordered_map>

namespace Game {
  using Definitions = TypeList<Terrain>;
  using Catalog     = BasicCatalog<Definitions>;

  using Components = TypeList<Pose>;
  using Registry   = BasicRegistry<Entity, Components>;

  struct World
  {
    Catalog  content;
    Registry entities;
    Chunk    environment;

    [[nodiscard]] Handle<Entity> find(Entity e) const;

    [[nodiscard]] Handle<Entity> create();

    void advance();

  private:

    u64                                        _entity_counter;
    std::unordered_map<Entity, Handle<Entity>> _lookup;
  };
}
