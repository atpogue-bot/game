#pragma once
#include "core/types.hh"

namespace Game {
  struct World;

  // Goal: stable, persistent, cross-session, unique identifier for a simulation object.
  enum class Entity : u64 { Nil = UINT64_MAX };

  // [[nodiscard]] bool is_valid(World&, Handle<Entity>);

  // [[nodiscard]] Entity get(World&, Handle<Entity>);

  // [[nodiscard]] Entity try_get(World&, Handle<Entity>);

  // [[nodiscard]] Handle<Entity> find_entity(World&, Entity);

  // [[nodiscard]] Handle<Entity> spawn(World&);
}
