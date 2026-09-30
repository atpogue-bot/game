#pragma once
#include <expected>
#include <string>

template <typename Type>
using Result = std::expected<Type, std::string>;
using Error  = std::unexpected<std::string>;
using Status = Result<void>;

#define TRY(expr)                 \
  ({                              \
    auto _result = (expr);        \
    if (!_result) return _result; \
    std::move(*_result);          \
  })
