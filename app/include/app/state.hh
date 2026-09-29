#pragma once
#include "app/pilot.hh"
#include "core/basic-catalog.hh"
#include "core/result.hh"
#include "game/world.hh"
#include "gfx/camera2D.hh"
#include "gfx/renderer.hh"
#include "gfx/texture.hh"
#include "sys/window.hh"
#include <memory>
#include <vector>

union SDL_Event;
struct SDL_Texture;
struct CommandBuffer;

namespace App {
  using Assets  = TypeList<Texture>;
  using Catalog = BasicCatalog<Assets>;

  struct Player
  {
    Entity                 entity;
    std::unique_ptr<Pilot> pilot;
    Camera2D               camera;
  };

  struct State
  {
    State()                            = default;
    State(State&&) noexcept            = default;
    State(State const&)                = delete;
    State& operator=(State&&) noexcept = default;
    State& operator=(State const&)     = delete;

    Result<void> load(World&, Entity player);
    void         handle_event(SDL_Event const&);
    void         step(World const&, CommandBuffer&);
    void         update(World const&, f32 delta);
    void         render(World const&, f32 alpha);

  private:
    Player   _player;
    Window   _window;
    Renderer _renderer;
    Catalog  _textures;
  };
}
