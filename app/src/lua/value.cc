#include "core/color.hh"
#include "core/rectangle.hh"
#include "sdk/state.hh"
#include "sdk/table.hh"
#include "sdk/value.hh"
#include <lua.hpp>
#include <print>

namespace Lua {

  Value::Value(Handle handle) noexcept : Handle(std::move(handle)) {}

  bool Value::to_boolean() const noexcept
  {
    lua_State* const L = state();
    push();
    bool boolean = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return boolean;
  }

  f64 Value::to_number() const noexcept
  {
    lua_State* const L = state();
    push();
    f64 number = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return number;
  }

  i64 Value::to_integer() const noexcept
  {
    lua_State* const L = state();
    push();
    i64 integer = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return integer;
  }

  std::string Value::to_string() const
  {
    lua_State* const L = state();
    push();
    std::string string = lua_tostring(L, -1);
    lua_pop(L, 1);
    return string;
  }

  Status Value::expect(Type expected) const
  {
    if (type() == expected) return {};
    Error e{ "expected " };
    e.error() += type_name(Type::Table);
    e.error() += ", found ";
    e.error() += describe();
    return e;
  }

  std::string Value::describe() const
  {
    std::string description{ type_name(type()) };
    switch (type()) {
    case Type::Boolean: description += (to_boolean() ? "true" : "false"); break;
    case Type::Integer: description += std::to_string(to_integer()); break;
    case Type::Number:  description += std::to_string(to_number()); break;
    case Type::String:
      description += '"';
      description += to_string();
      description += '"';
      break;
    default: break;
    }
    return description;
  }

  bool read(Value const& src, bool& dst)
  {
    auto boolean = src.expect_boolean();
    if (!boolean) return false;
    dst = *boolean;
    return true;
  }

  bool read(Value const& src, Integer& dst)
  {
    auto integer = src.expect_integer();
    if (!integer) return false;
    dst = *integer;
    return true;
  }

  bool read(Value const& src, Number& dst)
  {
    auto number = src.expect_number();
    if (!number) return false;
    dst = *number;
    return true;
  }

  bool read(Value const& src, std::string& dst)
  {
    auto string = src.expect_string();
    if (!string) return false;
    dst = std::string(*string);
    return true;
  }

  bool read(Value const& value, Rectangle& rect)
  {
    Table table = ({
      auto table = value.expect_table();
      if (!table) return false;
      std::move(*table);
    });

    bool ok  = true;
    ok      &= table["x"].read(rect.origin.x);
    ok      &= table["y"].read(rect.origin.y);
    ok      &= table["w"].read(rect.extent.x);
    ok      &= table["h"].read(rect.extent.y);
    return ok;
  }

  bool read(Value const& value, Color& color)
  {
    switch (value.type()) {
    case Lua::Type::Integer:
      {
        u32 rgba;
        if (!value.read(rgba)) return false;
        color = make_color_hex(rgba);
        break;
      }
    case Lua::Type::Table:
      {
        Lua::Table table = value.to_table();

        bool ok  = true;
        ok      &= table["r"].read(color.r);
        ok      &= table["g"].read(color.g);
        ok      &= table["b"].read(color.b);
        ok      &= table["a"].read(color.a);
        if (!ok) return false;
        break;
      }
    default:
      std::println("{}: expected integer or table, found {}", value.path(), value.describe());
      return false;
    }
    return true;
  }

} // namespace Lua

// Result<Integer> Value::expect_bounded_integer(Integer min, Integer max) const {}

// Result<Number> Value::expect_bounded_number(Number min, Number max) const
// {
//   f64 number = TRY(expect_number());
//   if (number > max || number < min) {
//     // return range_error_message(
//     //   src.type(), std::numeric_limits<T>::lowest(), std::numeric_limits<T>::max(), number);
//   }
// }

