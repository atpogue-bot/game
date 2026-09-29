#pragma once
#include "sdk/types.hh"

namespace Lua {

  struct State;

  // Must not outlive the state that created it.
  struct Reference
  {
    Reference() noexcept;
    Reference(State* state, i32 ridx) noexcept;
    Reference(Reference const&) noexcept;
    Reference(Reference&&) noexcept;
    Reference& operator=(Reference const&) noexcept;
    Reference& operator=(Reference&&) noexcept;
    ~Reference() noexcept;

    // Returns true if both refer to the same Lua value.
    friend bool operator==(Reference const& l, Reference const& r) noexcept;

    friend bool operator!=(Reference const& l, Reference const& r) noexcept
    {
      return !(operator==(l, r));
    }

    [[nodiscard]] Type type() const noexcept { return _type; }

    [[nodiscard]] State& state() const noexcept { return *_state; }

    // Pushes the referenced value onto the top of the stack.
    int push() const noexcept;

  private:
    State* _state; // non-owning pointer
    int    _ridx;
    Type   _type;
  };

} // namespace Lua
