#include "core/defer.hh"
#include "core/panic.hh"
#include "sdk/reference.hh"
#include "sdk/state.hh"
#include <format>
#include <lauxlib.h>
#include <lua.h>
#include <lua.hpp>
#include <string>
#include <variant>

namespace Lua {
  static int traceback(lua_State* L)
  {
    char const* message = lua_tostring(L, 1);
    if (message == nullptr) return 1;
    luaL_traceback(L, L, message, 1);
    return 1;
  }

  static void open_libraries(lua_State* L)
  {
    struct Library
    {
      char const*   name;
      lua_CFunction open;
    };

    constexpr Library libraries[] = {
      { LUA_GNAME, luaopen_base },        { LUA_TABLIBNAME, luaopen_table },
      { LUA_STRLIBNAME, luaopen_string }, { LUA_MATHLIBNAME, luaopen_math },
      { LUA_UTF8LIBNAME, luaopen_utf8 },
    };

    for (Library const& library : libraries) {
      luaL_requiref(L, library.name, library.open, 1);
      lua_pop(L, 1);
    }

    constexpr char const* unavailable[] = { "collectgarbage", "dofile", "load", "loadfile" };
    for (char const* name : unavailable) {
      lua_pushnil(L);
      lua_setglobal(L, name);
    }
  }

  // Ignores meta-methods in the global table.
  static void push_global(lua_State* L, std::string_view name) noexcept
  {
    lua_pushglobaltable(L);
    lua_pushlstring(L, name.data(), name.size());
    lua_rawget(L, -2);
    lua_replace(L, -2);
  }

  static std::string pop_string(lua_State* L)
  {
    size_t      length = 0;
    char const* data   = lua_tolstring(L, -1, &length);
    std::string string(data, length);
    lua_pop(L, 1);
    return string;
  }

  static Status call(lua_State* L)
  {
    lua_pushcfunction(L, traceback);
    auto errh = lua_gettop(L);
    DEFER(lua_remove(L, errh));
    int const status = lua_pcall(L, 0, 0, errh);
    if (status != LUA_OK) return Error(pop_string(L));
    return {};
  }

  Result<State> State::create()
  {
    lua_State* L = luaL_newstate();
    if (L == nullptr) return Error("failed to create Lua state");
    open_libraries(L);
    return State(L);
  }

  State::State(State&& other) noexcept : _handle{ other._handle } { other._handle = nullptr; }

  State& State::operator=(State&& other) noexcept
  {
    if (&other == this) return *this;
    _handle       = other._handle;
    other._handle = nullptr;
    return *this;
  }

  State::~State() noexcept { lua_close(_handle); }

  Table State::globals() noexcept { return Table(_handle, LUA_RIDX_GLOBALS); }

  Table State::create_table(LuaPath const& path) noexcept
  {
    lua_newtable(_handle);
    return Table(_handle, luaL_ref(_handle, LUA_REGISTRYINDEX));
  }

  Status State::load(std::string_view path)
  {
    auto status = luaL_loadfile(_handle, std::string(path).c_str());
    if (status != LUA_OK) return Error(pop_string(_handle));
    return call(_handle);
  }

  Status State::execute(std::string_view name, std::string_view source)
  {
    auto status = luaL_loadbuffer(_handle, source.data(), source.size(), std::string(name).c_str());
    if (status != LUA_OK) return Error(pop_string(_handle));
    return call(_handle);
  }

} // namespace Lua
