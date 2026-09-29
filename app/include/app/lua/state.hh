#pragma once

#include "core/result.hh"
#include "sdk/table.hh"
#include "sdk/value.hh"
#include <string_view>

struct lua_State;

namespace Lua {

  struct State
  {
    [[nodiscard]] static Result<State> create();

    State()                        = default;
    State(State const&)            = delete;
    State& operator=(State const&) = delete;

    State(State&&) noexcept;
    State& operator=(State&&) noexcept;
    ~State() noexcept;

    [[nodiscard]] Status load(std::string_view file);

    [[nodiscard]] Status execute(std::string_view name, std::string_view source);

    [[nodiscard]] Table globals() noexcept;

    [[nodiscard]] Table create_table() noexcept;

    [[nodiscard]] lua_State* get() const noexcept { return _handle; }

  private:
    explicit State(lua_State*) noexcept;

    lua_State* _handle;
  };

} // namespace Lua
