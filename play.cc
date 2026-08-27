// A boids demo.
//
// One chunk of grassland with two flocks over it. Each bird steers only by what its own flockmates
// are doing nearby -- separation, alignment, cohesion, and an urge to turn back at the edge of the
// chunk -- and the shape of the flock is whatever those add up to. The flocks are tuned in
// `content/boids.lua`; nothing here knows how a bird flies.
//
// WASD moves the camera, the mouse wheel zooms, Q or escape quits.

#include "app/application.hh"
#include "core/clock.hh"
#include "core/panic.hh"
#include "core/random.hh"
#include "game/biome/grassland.hh"
#include "game/catalog.hh"
#include "game/chunk.hh"
#include "game/command-buffer.hh"
#include "game/component/pose.hh"
#include "game/context.hh"
#include "game/entity.hh"
#include "game/flock.hh"
#include "game/flocking.hh"
#include "game/load.hh"
#include "game/simulation.hh"
#include "game/types.hh"
#include "sys/main.hh"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <memory>
#include <print>
#include <random>

// The one chunk that makes up the demo's world, and so the area the flocks are kept inside.
constexpr Rectangle world_bounds{ { 0.f, 0.f }, { f32(chunk_size), f32(chunk_size) } };

// How many birds of each flock to release. Small enough to see individuals, large enough that the
// flock has a shape of its own.
constexpr u32 starling_count = 220u;
constexpr u32 gull_count     = 40u;

struct Runtime
{
  Simulation    sim;
  Application   app;
  CommandBuffer cmds;
  Flocking      flocking{ world_bounds };
  Clock         clock = { 0, 32 };
};

void load_chunk(LoadContext ctx, u64 seed, Chunk& chunk)
{
  SplitMix64                         rng(seed);
  std::uniform_int_distribution<u32> dist{ 0u, chunk_size - 1u };
  GrasslandGenerator(ctx, seed).generate(dist(rng), dist(rng), chunk);
}

Entity load_player(Context ctx)
{
  auto player = create_entity(ctx);
  player.emplace<Pose>(glm::vec2{ chunk_size * 0.5f, chunk_size * 0.5f });
  INVARIANT(find_entity(ctx, player.id()) != Handle<Entity>::null());
  return player.id();
}

// Release both flocks over the middle of the chunk. Everything about the demo that is not the
// world seed is fixed, so the same seed always produces the same first few seconds of flight.
bool load_flocks(Context ctx, u64 seed)
{
  auto const   catalog   = access_catalog(ConstContext(ctx));
  Token<Flock> starlings = catalog.find<Flock>("starlings");
  Token<Flock> gulls     = catalog.find<Flock>("gulls");
  if (!starlings || !gulls) return false;

  constexpr f32   inset = chunk_size * 0.25f;
  Rectangle const nursery{ { inset, inset },
                           { chunk_size - 2.f * inset, chunk_size - 2.f * inset } };
  spawn_flock(ctx, starlings, starling_count, seed, nursery);
  spawn_flock(ctx, gulls, gull_count, seed, nursery);
  return true;
}

Runtime* start(int /*argc*/, char* /*argv*/[])
{
  auto state = std::make_unique<Runtime>();
  state->clock.set_rate(32);

  u64 const seed = random_seed();

  auto ctx = state->sim.load();
  if (!ctx) return nullptr;
  if (!load_content(*ctx, "content/terrain.lua") || !load_content(*ctx, "content/boids.lua")) {
    std::println("Failed to load content.");
    return nullptr;
  }
  load_chunk(*ctx, seed, state->sim.scene());
  if (!load_flocks(*ctx, seed)) {
    std::println("Failed to find the demo's flocks in content/boids.lua.");
    return nullptr;
  }
  if (auto result = state->app.load(*ctx, load_player(*ctx)); !result) {
    std::println("Failed to open application: {}", result.error().msg);
    return nullptr;
  }
  return state.release();
}

void step(Runtime& state, i64 tick)
{
  auto ctx = state.sim.step(tick);
  state.app.step(state.cmds, ctx);
  state.cmds.dispatch(ctx);
  // Simulation systems run on the state the commands left behind, and on the fixed timestep: the
  // flock is reproducible only because it never sees a variable frame time.
  state.flocking.step(ctx, state.clock.period());
}

void update(Runtime& state, f32 delta)
{
  auto ctx = state.sim.context();
  state.app.update(ctx, delta);
}

void render(Runtime& state, f32 alpha)
{
  auto ctx = state.sim.context();
  state.app.render(ctx, alpha);
}

void iterate(Runtime& state)
{
  auto tick = state.clock.tick();
  for (auto steps = state.clock.advance(); steps > 0; steps--) step(state, tick++);
  DEBUG_ASSERT(tick == state.clock.tick());

  update(state, state.clock.delta());

  render(state, state.clock.alpha());
}

void handle_event(Runtime& state, SDL_Event const& event) { state.app.handle_event(event); }

void quit(Runtime* state) { delete state; }
