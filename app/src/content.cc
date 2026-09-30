#include "app/content.hh"
#include "app/lua/state.hh"
#include "app/lua/table.hh"
#include "app/lua/value.hh"
#include "core/panic.hh"
#include "game/world.hh"
#include "gfx/renderer.hh"
#include "gfx/surface.hh"
#include <optional>
#include <print>

namespace Lua {
  // Reads an array of the form `{ x, y, width, height }`.
  static bool read(Rectangle& dst, Value const& src)
  {
    std::optional<Table> table = src.expect_table();
    if (!table) return false;

    bool ok  = true;
    ok      &= (*table)[1].read(dst.origin.x);
    ok      &= (*table)[2].read(dst.origin.y);
    ok      &= (*table)[3].read(dst.extent.x);
    ok      &= (*table)[4].read(dst.extent.y);
    return ok;
  }

  // Reads either a hex integer of the form `0xRRGGBBAA` or a table of the form `{ r, g, b, a }`.
  static bool read(Color& dst, Value const& src)
  {
    switch (src.type()) {
    case Type::Integer:
      {
        u32 rgba = 0u;
        if (!src.read(rgba)) return false;
        dst = make_color_hex(rgba);
        return true;
      }
    case Type::Table:
      {
        Table const table = src.to_table();

        bool ok  = true;
        ok      &= table["r"].read(dst.r);
        ok      &= table["g"].read(dst.g);
        ok      &= table["b"].read(dst.b);
        ok      &= table["a"].read(dst.a);
        return ok;
      }
    default:
      std::println("{}: expected integer or table, found {}", src.path(), src.describe());
      return false;
    }
  }
}

namespace App {
  namespace {
    struct Loader
    {
      Game::World& world;
      Renderer&    renderer;
      Visuals&     visuals;
    };
  }

  // Finds the texture loaded from the path, loading it if it hasn't been already.
  static std::optional<Handle<Texture>> import_texture(Loader& loader, Lua::Value const& value)
  {
    std::optional<std::string_view> path = value.expect_string();
    if (!path) return std::nullopt;

    Handle<Texture> const handle = loader.visuals.assets.find<Texture>(*path);
    if (handle) return handle;

    Result<Surface> surface = load_image(*path);
    if (!surface) {
      std::println("{}: {}", value.path(), surface.error());
      return std::nullopt;
    }
    Result<Texture> texture = loader.renderer.create_texture(*surface);
    if (!texture) {
      std::println("{}: {}", value.path(), texture.error());
      return std::nullopt;
    }
    return loader.visuals.assets.emplace<Texture>(*path, std::move(*texture));
  }

  static bool import_sprite(Loader& loader, Lua::Value const& value, Sprite& sprite)
  {
    std::optional<Lua::Table> table = value.expect_table();
    if (!table) return false;

    bool ok = true;
    if (std::optional<Handle<Texture>> atlas = import_texture(loader, (*table)["atlas"])) {
      sprite.atlas = *atlas;
    } else ok = false;
    ok &= (*table)["source"].read(sprite.source);
    ok &= (*table)["tint"].read(sprite.tint);
    return ok;
  }

  static bool import_terrain(Loader& loader, Lua::Table const& definitions)
  {
    bool ok = true;
    for (Lua::Key const key : definitions.keys()) {
      Lua::Value const value = definitions[key];
      if (!key.is_name()) {
        std::println("{}: terrain must be labeled by a string", value.path());
        ok = false;
        continue;
      }

      std::optional<Lua::Table> definition = value.expect_table();
      if (!definition) {
        ok = false;
        continue;
      }

      Game::Terrain terrain;
      Sprite        sprite;
      if (!import_sprite(loader, (*definition)["sprite"], sprite)) {
        ok = false;
        continue;
      }

      Handle<Game::Terrain> const handle
        = loader.world.content.emplace<Game::Terrain>(key.as_name(), terrain);
      INVARIANT(
        handle.index == loader.visuals.terrain.size(),
        "terrain visuals must be parallel to terrain definitions");
      loader.visuals.terrain.push_back(sprite);
    }
    return ok;
  }

  // Reads the scalar half of a flock definition: everything but the sprite is a plain number.
  static bool import_flock_rules(Lua::Table const& definition, Game::Flock& flock)
  {
    bool ok  = true;
    ok      &= definition["vision"].read(flock.vision);
    ok      &= definition["personal_space"].read(flock.personal_space);
    ok      &= definition["min_speed"].read(flock.min_speed);
    ok      &= definition["max_speed"].read(flock.max_speed);
    ok      &= definition["max_force"].read(flock.max_force);
    ok      &= definition["separation"].read(flock.separation);
    ok      &= definition["alignment"].read(flock.alignment);
    ok      &= definition["cohesion"].read(flock.cohesion);
    ok      &= definition["containment"].read(flock.containment);
    ok      &= definition["margin"].read(flock.margin);
    if (!ok) return false;

    // The steering system divides by these, and a flock that cannot see or cannot turn is not a
    // flock. Catch it here, while the definition's path is still to hand.
    if (flock.vision <= 0.f || flock.max_force <= 0.f || flock.margin <= 0.f) {
      std::println("{}: vision, max_force and margin must be positive", definition.path());
      return false;
    }
    if (flock.min_speed < 0.f || flock.max_speed < flock.min_speed) {
      std::println("{}: expected 0 <= min_speed <= max_speed", definition.path());
      return false;
    }
    return true;
  }

  static bool import_flocks(Loader& loader, Lua::Table const& definitions)
  {
    bool ok = true;
    for (Lua::Key const key : definitions.keys()) {
      Lua::Value const value = definitions[key];
      if (!key.is_name()) {
        std::println("{}: a flock must be labeled by a string", value.path());
        ok = false;
        continue;
      }

      std::optional<Lua::Table> definition = value.expect_table();
      if (!definition) {
        ok = false;
        continue;
      }

      Game::Flock flock{};
      Sprite      sprite;
      if (!import_sprite(loader, (*definition)["sprite"], sprite)) {
        ok = false;
        continue;
      }
      if (!import_flock_rules(*definition, flock)) {
        ok = false;
        continue;
      }

      Handle<Game::Flock> const handle
        = loader.world.content.emplace<Game::Flock>(key.as_name(), flock);
      INVARIANT(
        handle.index == loader.visuals.flocks.size(),
        "flock visuals must be parallel to flock definitions");
      loader.visuals.flocks.push_back(sprite);
    }
    return ok;
  }

  bool load_content(std::string_view path, Game::World& world, Renderer& renderer, Visuals& visuals)
  {
    std::optional<Lua::State> lua = Lua::State::create();
    if (!lua) return false;

    Lua::Table const globals = lua->globals();
    {
      Lua::Table const content = lua->create_table();
      content.set("terrain", lua->create_table());
      content.set("flock", lua->create_table());
      globals.set("content", content);
    }

    if (!lua->load(path)) return false;

    // Read back through the globals in case the script replaced the tables.
    std::optional<Lua::Table> content = globals["content"].expect_table();
    if (!content) return false;
    std::optional<Lua::Table> terrain = (*content)["terrain"].expect_table();
    if (!terrain) return false;
    std::optional<Lua::Table> flock = (*content)["flock"].expect_table();
    if (!flock) return false;

    Loader loader{ .world = world, .renderer = renderer, .visuals = visuals };
    bool   ok  = import_terrain(loader, *terrain);
    ok        &= import_flocks(loader, *flock);
    return ok;
  }
}
