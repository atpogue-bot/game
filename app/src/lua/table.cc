#include "core/panic.hh"
#include "sdk/state.hh"
#include "sdk/table.hh"
#include <lua.h>
#include <lua.hpp>
#include <string_view>

namespace Lua {
  static void push_key(lua_State* L, Key key) noexcept
  {
    if (key.is_name()) {
      auto str = key.as_name();
      lua_pushlstring(L, str.data(), str.size());
    } else lua_pushinteger(L, key.as_index());
  }

  Table::Table(Reference reference) noexcept : Reference(std::move(reference))
  {
    DEBUG_ASSERT(reference.type() == Type::Table);
  }

  void Table::set(Key key, bool value) const noexcept
  {
    lua_State* const L   = state();
    int const        idx = push();
    push_key(L, key);
    lua_pushboolean(L, value);
    lua_rawset(L, idx);
    lua_pop(L, 1);
  }

  void Table::set(Key key, f64 value) const noexcept
  {
    lua_State* const L   = state();
    int const        idx = push();
    push_key(L, key);
    lua_pushnumber(L, value);
    lua_rawset(L, idx);
    lua_pop(L, 1);
  }

  void Table::set(Key key, i64 value) const noexcept
  {
    lua_State* const L   = state();
    int const        idx = push();
    push_key(L, key);
    lua_pushinteger(L, value);
    lua_rawset(L, idx);
    lua_pop(L, 1);
  }

  void Table::set(Key key, std::string_view value) const noexcept
  {
    lua_State* const L   = state();
    int const        idx = push();
    push_key(L, key);
    lua_pushlstring(L, value.data(), value.size());
    lua_rawset(L, idx);
    lua_pop(L, 1);
  }

  void Table::set(Key key, Reference const& value) const noexcept
  {
    PRECONDITION(value.state() == state());
    lua_State* const L   = state();
    int const        idx = push();
    push_key(L, key);
    value.push();
    lua_rawset(L, idx);
    lua_pop(L, 1);
  }

  void Table::erase(Key key) const noexcept
  {
    lua_State* const L   = state().get();
    int const        idx = push();
    push_key(L, key);
    lua_pushnil(L);
    lua_rawset(L, idx);
    lua_pop(L, 1);
  }

  Value Table::operator[](Key key) const noexcept
  {
    lua_State* const L   = state().get();
    int const        idx = push();
    push_key(L, key);
    lua_rawget(L, idx);
    return Value({ &state(), luaL_ref(L, LUA_REGISTRYINDEX) });
  }

  Status Table::expect_table(Key key) const
  {
    if (type() != Type::Table) return type_error(Type::Table, type());
    return {};
  }

} // namespace Lua
