#pragma once
#include "core/types.hh"
#include "sdk/reference.hh"
#include "sdk/types.hh"
#include <concepts>
#include <limits>
#include <optional>
#include <string>

struct Rectangle;
struct Color;

namespace Lua {
  struct Table;
  struct Value;

  template <typename T>
  concept ValueReadable = requires (T& dst, Value const& src) {
    { read(dst, src) } -> std::same_as<bool>;
  };

  bool read(bool& dst, Value const& srct);

  bool read(std::string& dst, Value const& srct);

  template <std::floating_point T>
  bool read(T& dst, Value const& srct);

  template <std::integral T>
  bool read(T& dst, Value const& src);

  // Must not outlive the state that created it.
  struct Value
  {
    Value(Reference, std::string path) noexcept;

    // Problem: accumulate multiple errors and provide full diagnostic
    // Problem: diagnostic should include path
    // Problem: diagnostic can include warnings on happy path
    template <ValueReadable T>
    [[nodiscard]] bool read(T& dst) const
    {
      return read(dst, *this);
    }

    // Intended to be called once the type is known (i.e. in a switch statement over type).
    // In debug: asserts on type mismatch.
    // In release: same behavior as equivalent lua C API.
    [[nodiscard]] bool             to_boolean() const noexcept;
    [[nodiscard]] f64              to_number() const noexcept;
    [[nodiscard]] i64              to_integer() const noexcept;
    [[nodiscard]] std::string_view to_string() const noexcept;
    [[nodiscard]] Table            to_table() const noexcept;

    [[nodiscard]] std::optional<bool>    expect_boolean() const;
    [[nodiscard]] std::optional<Integer> expect_integer() const;
    [[nodiscard]] std::optional<Integer> expect_integer_range(Integer min, Integer max) const;
    [[nodiscard]] std::optional<Number>  expect_number() const;
    [[nodiscard]] std::optional<Number>  expect_number_range(Number min, Number max) const;
    [[nodiscard]] std::optional<std::string_view> expect_string() const;
    [[nodiscard]] std::optional<Table>            expect_table() const;

    [[nodiscard]] Type type() const noexcept { return _reference.type(); }

    [[nodiscard]] bool is_nil() const noexcept { return type() == Type::Nil; }

    [[nodiscard]] bool is_boolean() const noexcept { return type() == Type::Boolean; }

    [[nodiscard]] bool is_integer() const noexcept { return type() == Type::Integer; }

    [[nodiscard]] bool is_number() const noexcept { return type() == Type::Number; }

    [[nodiscard]] bool is_string() const noexcept { return type() == Type::String; }

    [[nodiscard]] bool is_table() const noexcept { return type() == Type::Table; }

    [[nodiscard]] std::string describe() const;

    [[nodiscard]] std::string_view path() const { return _path; }

  private:
    friend Table;

    Reference   _reference;
    std::string _path;
  };

  bool read(bool& dst, Value const& value);
  bool read(Integer& dst, Value const& value);
  bool read(Number& dst, Value const& value);
  bool read(std::string& dst, Value const& value);

  template <std::integral T>
  bool read(T& dst, Value const& src)
  {
    auto integer
      = src.expect_integer_range(std::numeric_limits<T>::lowest(), std::numeric_limits<T>::max());
    if (!integer) return false;
    dst = static_cast<T>(*integer);
    return true;
  }

  template <std::floating_point T>
  bool read(T& dst, Value const& src)
  {
    auto number
      = src.expect_number_range(std::numeric_limits<T>::lowest(), std::numeric_limits<T>::max());
    if (!number) return false;
    dst = static_cast<T>(*number);
    return true;
  }

  bool read(Rectangle& rect, Value const& src);
  bool read(Color& color, Value const& src);

} // namespace Lua

