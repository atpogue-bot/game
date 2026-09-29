#include "core/panic.hh"
#include "game/world.hh"

Handle<Entity> World::create()
{
  auto const id     = Entity{ _entity_counter++ };
  auto const handle = _entities.create(id);
  auto [_, success] = _lookup.emplace(id, handle);
  INVARIANT(success);
  return handle;
}

Handle<Entity> World::find_entity(Entity e) const
{
  auto it = _lookup.find(e);
  return it != _lookup.end() ? it->second : Handle<Entity>::null();
}

void World::advance() {}

