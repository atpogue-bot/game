#pragma once

// TODO: ability to iterate over all non-nil fields in the table

#include "sdk/key.hh"
#include "sdk/reference.hh"
#include "sdk/value.hh"
#include <vector>

namespace Lua {

  struct Table;

  template <typename T>
  concept TableReadable = requires (T& dst, Table const& src) {
    { read(dst, src) } -> std::same_as<bool>;
  };

  // Must not outlive the state that created it.
  // Ignores metamethods...
  struct Table
  {
    Table(Reference, std::string path) noexcept;

    template <TableReadable T>
    [[nodiscard]] bool read(T& dst) const
    {
      return read(dst, *this);
    }

    // Returns a nil-value object on missing field
    [[nodiscard]] Value operator[](Key key) const noexcept;

    void set(Key key, bool value) const noexcept;
    void set(Key key, f64 value) const noexcept;
    void set(Key key, i64 value) const noexcept;
    void set(Key key, std::string_view value) const noexcept;

    void set(Key key, Table const& table) const noexcept { set(key, table._reference); }

    void set(Key key, Value const& value) const noexcept { set(key, value._reference); }

    // Assumes that the handle was produced by the same state.
    void set(Key key, Reference const& handle) const noexcept;

    void erase(Key key) const noexcept;

    [[nodiscard]] std::vector<Key> keys() const;

    [[nodiscard]] std::string_view path() const { return _path; }

  private:
    Reference   _reference;
    std::string _path;
  };

} // namespace Lua
