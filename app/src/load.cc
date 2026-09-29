#include "game/content.hh"
#include "game/context.hh"
#include "game/sprite.hh"
#include "game/terrain.hh"
#include "game/types.hh"
#include "sdk/node.hh"
#include "sdk/state.hh"
#include "sdk/table.hh"
#include "sdk/value.hh"
#include <print>

static bool compile_terrain(Lua::Table& table, CatalogWriter catalog)
{
  bool    ok = true;
  Terrain terrain;
  for (auto key : table.keys()) {
    terrain = {};
    if (key.is_index()) continue;
    ok = table[key].read(terrain);
    catalog.emplace<Terrain>(key.as_name(), terrain);
  }
  return ok;
}

static void init_lua(Lua::State& lua)
{
  Lua::Table const G       = lua.globals();
  Lua::Table const content = lua.create_table();

  G.set("content", content);
  content.set("terrain", lua.create_table());
};

bool load_content(Content&, LuaTable)
{
  auto lua = Lua::State::create();
  if (!lua) return false;
  init_lua(*lua);

  if (!lua->load("content/terrain.lua")) return false;

  Lua::Table const G = lua->globals();

  auto content_table = G["content"].expect_table();
  if (!content_table) return false;

  Content content;
  content_table->

    std::println("Failed to compile content!");
  return read()

    return compile_content(*content, access_catalog(context));
}

// upvalues? requires parsing to be on call:
//     type label {...}
// DTOs?
// extra space?
