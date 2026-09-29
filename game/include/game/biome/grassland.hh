#pragma once
#include "game/chunk.hh"

namespace Game {
  struct GrasslandGenerator : ChunkGenerator
  {
    GrasslandGenerator(World&, u64 seed);
    void generate(u32 x, u32 y, Chunk& chunk) override;

  private:

    u64 const             seed_;
    Handle<Terrain> const terrain_[6];
  };
}
