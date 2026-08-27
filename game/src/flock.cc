#include "game/flock.hh"
#include "core/defer.hh"
#include "game/catalog.hh"
#include "game/lua.hh"
#include "game/sprite.hh"
#include "game/types.hh"
#include <lua.hpp>

// Everything but the sprite is a plain number, so the scalar half of a flock definition is
// described once here and parsed generically. Every field is required: a half-tuned flock is a
// content error, not a flock with defaults.
static constexpr lua::Field<Flock> flock_schema[] = {
  { "vision", &Flock::vision },           { "personal_space", &Flock::personal_space },
  { "min_speed", &Flock::min_speed },     { "max_speed", &Flock::max_speed },
  { "max_force", &Flock::max_force },     { "separation", &Flock::separation },
  { "alignment", &Flock::alignment },     { "cohesion", &Flock::cohesion },
  { "containment", &Flock::containment }, { "margin", &Flock::margin },
};

// flock "name" { sprite = {}, vision = N, ... }
static int parse_flock_table(lua_State* L)
{
  INVARIANT(lua_islightuserdata(L, lua_upvalueindex(1)));
  INVARIANT(lua_isstring(L, lua_upvalueindex(2)));
  auto catalog = static_cast<CatalogWriter*>(lua_touserdata(L, lua_upvalueindex(1)));
  auto name    = lua_tostring(L, lua_upvalueindex(2));
  do {
    // arg 1: definition table
    if (!lua_istable(L, 1)) {
      lua::push_fstring(
        L, "flock '{}': expected table, found {}", name, lua_typename(L, lua_type(L, 1)));
      break;
    }

    int  flock  = lua_gettop(L);
    auto sprite = lua::try_get_sprite(*catalog, L, flock, "sprite");
    if (!sprite) {
      lua::push_string(L, sprite.error().msg);
      break;
    }
    auto fields = lua::try_get_fields<Flock>(L, flock, flock_schema);
    if (!fields) {
      lua::push_string(L, fields.error().msg);
      break;
    }
    // The steering system divides by these, and a flock that cannot see or cannot turn is not a
    // flock. Catch it here, while there is still a definition name to put in the message.
    if (fields->vision <= 0.f || fields->max_force <= 0.f || fields->margin <= 0.f) {
      lua::push_fstring(L, "flock '{}': vision, max_force and margin must be positive", name);
      break;
    }
    if (fields->min_speed < 0.f || fields->max_speed < fields->min_speed) {
      lua::push_fstring(L, "flock '{}': expected 0 <= min_speed <= max_speed", name);
      break;
    }

    DEFER(lua_pop(L, 1));
    fields->sprite = *sprite;
    catalog->emplace<Flock>(name, *fields);
    return 0;
  } while (false);
  // this will unwind the stack without calling C++ destructors
  return lua_error(L);
}

static int build_flock(lua_State* L)
{
  INVARIANT(lua_islightuserdata(L, lua_upvalueindex(1)));
  // arg 1: name string
  if (!lua_isstring(L, 1)) {
    lua_pushstring(
      L, std::format("flock: expected string, found {}", lua_typename(L, lua_type(L, 1))).data());
    return lua_error(L);
  }
  lua_pushvalue(L, lua_upvalueindex(1));
  lua_pushvalue(L, 1); // name
  lua_pushcclosure(L, parse_flock_table, 2);
  return 1;
}

void lua::add_flock_builder(lua_State* L, CatalogWriter& catalog)
{
  lua_pushlightuserdata(L, &catalog);
  lua_pushcclosure(L, build_flock, 1);
  lua_setglobal(L, "flock");
}
