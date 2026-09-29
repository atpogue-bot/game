#include "game/world.hh"
#include "sdk/lua/value.hh"

bool read(World& world, Value const& value, Sprite& sprite)
{
  Table table = ({
    auto table = value.expect_table();
    if (!table) return false;
    std::move(*table);
  });

  bool ok  = true;
  ok      &= table["atlas"].read(sprite.atlas);
  ok      &= table["source"].read(sprite.source);
  ok      &= table["tint"].read(sprite.tint);
  return ok;
}
