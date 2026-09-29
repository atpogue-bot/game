#include "core/panic.hh"
#include "sdk/reference.hh"
#include "sdk/state.hh"
#include <lua.hpp>

namespace Lua {

  static Type get_type(lua_State* L, int idx) noexcept
  {
    if (lua_isinteger(L, idx)) return Type::Integer;
    return static_cast<Type>(idx);
  }

  Reference::Reference(State* state, i32 ridx) noexcept
    : _state{ state }, _ridx{ ridx }, _type{ Type::None }
  {
    PRECONDITION(_state != nullptr);
    lua_State* const L = _state->handle();
    lua_rawgeti(L, LUA_REGISTRYINDEX, _ridx);
    _type = get_type(L, -1);
    lua_pop(L, 1);
  }

  Reference::Reference(Reference const& other) noexcept
    : _state{ other._state }, _ridx{ LUA_NOREF }, _type{ Type::None }
  {
    lua_State* const L = _state->handle();
    lua_rawgeti(L, LUA_REGISTRYINDEX, other._ridx);
    _type = get_type(L, -1);
    _ridx = luaL_ref(L, LUA_REGISTRYINDEX);
  }

  Reference::Reference(Reference&& other) noexcept
    : _state{ other._state }, _ridx{ other._ridx }, _type{ other._type }
  {
    other._ridx = LUA_NOREF;
    other._type = Type::None;
  }

  Reference& Reference::operator=(Reference const& other) noexcept
  {
    if (&other == this) return *this;
    if (_ridx != LUA_NOREF) luaL_unref(_state->handle(), LUA_REGISTRYINDEX, _ridx);
    _state             = other._state;
    lua_State* const L = _state->handle();
    lua_rawgeti(L, LUA_REGISTRYINDEX, other._ridx);
    _type = get_type(L, -1);
    _ridx = luaL_ref(L, LUA_REGISTRYINDEX);
    return *this;
  }

  Reference& Reference::operator=(Reference&& other) noexcept
  {
    if (&other == this) return *this;
    if (_ridx != LUA_NOREF) luaL_unref(_state->handle(), LUA_REGISTRYINDEX, _ridx);
    _state      = other._state;
    _ridx       = other._ridx;
    _type       = other._type;
    other._ridx = LUA_NOREF;
    other._type = Type::None;
    return *this;
  }

  Reference::~Reference() noexcept
  {
    lua_State* const L = _state->handle();
    if (_ridx != LUA_NOREF) {
      luaL_unref(L, LUA_REGISTRYINDEX, _ridx);
    }
  }

  bool operator==(Reference const& l, Reference const& r) noexcept
  {
    if (l._state != r._state) return false;
    lua_State* const L = l._state->handle();
    if (L == nullptr) return true;
    if (l._ridx == r._ridx) return true;
    lua_rawgeti(L, LUA_REGISTRYINDEX, l._ridx);
    lua_rawgeti(L, LUA_REGISTRYINDEX, r._ridx);
    bool out = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);
    return out;
  }

  int Reference::push() const noexcept
  {
    lua_State* const L = _state->handle();
    DEBUG_ASSERT(_ridx != LUA_NOREF);
    lua_rawgeti(L, LUA_REGISTRYINDEX, _ridx);
    DEBUG_ASSERT(_type == get_type(L, -1));
    return lua_gettop(L);
  }

} // namespace Lua
