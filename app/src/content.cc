#include "game/terrain.hh"
#include "sdk/content.hh"
#include "sdk/table.hh"

namespace sdk {

  static bool import_terrain(Content& content, Lua::Table const& root)
  {
    bool    ok = true;
    Terrain terrain;
    for (auto key : root.keys()) {
      if (key.is_index()) {
        ok &= false;
        std::println(
          "{}[{}]: terrain entries must be keyed by string", root.path(), key.as_index());
        continue;
      }

      auto record = root[key].expect_table();
      if (!record) {
        ok &= false;
        continue;
      }

      ok &= import(content, terrain, *record);
      define(content, key.as_name(), terrain);
    }
    return ok;
  }

  bool import(Content& content, Lua::Table const& root)
  {
    if (auto table = root["terrain"].expect_table()) import_terrain(content, *table);
  }

}
