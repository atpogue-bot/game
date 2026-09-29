#include "sdk/types.hh"
#include <format>
#include <initializer_list>

namespace Lua {

  static std::string list(std::initializer_list<Type> types)
  {
    auto        it = types.begin();
    std::string out(type_name(*it));
    while (++it != types.end()) {
      out += ", ";
      out += type_name(*it);
    }
    return out;
  }

  Error type_error(Type expected, Type found)
  {
    return Error{ std::format("expected {}, found {}", type_name(expected), type_name(found)) };
  }

  Error type_error(std::initializer_list<Type> expected, Type found)
  {
    return Error{ std::format("expected one of ({}), found {}", list(expected), type_name(found)) };
  }

  Error range_error(i64 min, i64 max, i64 found)
  {
    return Error{ std::format("expected integer in range [{}, {}], found {}", min, max, found) };
  }

  Error range_error(f64 min, f64 max, f64 found)
  {
    return Error{ std::format("expected number in range [{}, {}], found {}", min, max, found) };
  }

}
