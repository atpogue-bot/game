#pragma once
#include "app/sprite.hh"
#include "game/content/terrain.hh"

struct Texture;

namespace Lua {
  struct Table;

  bool import(Color&, Table const&);

  bool import(Game::Content&, Table const&);

  bool import(App::Assets&, App::Sprite&, Table const&);

  bool import(Game::Content&, Game::Terrain&, Table const&);

  bool read(Content&, Lua::Table const&, Sprite&);
}
