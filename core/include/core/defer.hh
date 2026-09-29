#pragma once
#include <concepts>

template <std::invocable F>
struct Defer
{
  F fn;

  ~Defer() { fn(); }
};

// deduction guide
template <typename F>
Defer(F) -> Defer<F>;

#define DEFER(expr)         \
  Defer _defer##__COUNTER__ \
  {                         \
    [&] { expr; }           \
  }
