#pragma once
#include "content/terrain.hh"
#include "core/types.hh"

namespace Game {
  struct Tile
  {
    Handle<Terrain> terrain;
    // u32 elevation;
    // u32 structure;
  };
}
