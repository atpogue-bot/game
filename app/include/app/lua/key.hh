#pragma once
#include "core/panic.hh"
#include "core/types.hh"
#include <string_view>

namespace Lua {

  // Note: Key does not own or manage a copy of the string value. It holds a string view.
  struct Key
  {
    enum class Kind : u8 { Index, Name };

    constexpr Key(char const* name) : _kind{ Kind::Name }, _name{ name } {}

    constexpr Key(std::string_view name) : _kind{ Kind::Name }, _name{ name } {}

    constexpr Key(int index) noexcept : _kind{ Kind::Index }, _index{ index } {}

    constexpr Key& operator=(std::string_view name)
    {
      _kind = Kind::Name;
      _name = name;
      return *this;
    }

    constexpr Key& operator=(int index) noexcept
    {
      _kind  = Kind::Index;
      _index = index;
      return *this;
    }

    [[nodiscard]] std::string_view as_name() const
    {
      DEBUG_ASSERT(_kind == Kind::Name);
      return std::string_view(_name.begin(), _name.end());
    }

    [[nodiscard]] int as_index() const noexcept
    {
      DEBUG_ASSERT(_kind == Kind::Index);
      return _index;
    }

    [[nodiscard]] int is_index() const noexcept { return _kind == Kind::Index; }

    [[nodiscard]] int is_name() const noexcept { return _kind == Kind::Name; }

    [[nodiscard]] Kind kind() const noexcept { return _kind; }

  private:
    Kind             _kind  = Kind::Index;
    int              _index = 0;
    std::string_view _name  = {};
  };

} // namespace Lua
