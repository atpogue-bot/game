// A boids demo.
//
// One chunk of grassland with two flocks over it. Each bird steers only by what its own flockmates
// are doing nearby -- separation, alignment, cohesion, and an urge to turn back at the edge of the
// chunk -- and the shape of the flock is whatever those add up to. The flocks are tuned in
// `content/boids.lua`; nothing here knows how a bird flies.
//
// WASD moves the camera, the mouse wheel zooms, Q or escape quits.

#include "app/state.hh"
#include "core/clock.hh"
#include "core/panic.hh"
#include "core/random.hh"
#include "game/area.hh"
#include "game/biome/grassland.hh"
#include "game/command-buffer.hh"
#include "game/content/flock.hh"
#include "game/flocking.hh"
#include "game/world.hh"
#include "sys/main.hh"
#include <memory>
#include <print>
#include <random>

// The one chunk that makes up the demo's world, and so the area the flocks are kept inside.
constexpr Game::Area environment_bounds{
  .min = { 0.f, 0.f },
  .max = { f32(Game::chunk_size), f32(Game::chunk_size) },
};

// How many birds of each flock to release. Small enough to see individuals, large enough that the
// flock has a shape of its own.
constexpr u32 starling_count = 220u;
constexpr u32 gull_count     = 40u;

struct Runtime
{
  Game::World         world;
  App::State          app;
  Game::CommandBuffer cmds;
  Game::Flocking      flocking{ environment_bounds };
  Clock               clock = { 0, 32 };
};

static void generate_environment(Game::World& world, u64 seed)
{
  SplitMix64                         rng(seed);
  std::uniform_int_distribution<u32> dist{ 0u, Game::chunk_size - 1u };
  Game::GrasslandGenerator(world, seed).generate(dist(rng), dist(rng), world.environment);
}

static Game::Entity spawn_player(Game::World& world)
{
  Handle<Game::Entity> const handle = world.create();
  world.entities.emplace<Game::Pose>(
    handle, glm::vec2{ Game::chunk_size * 0.5f, Game::chunk_size * 0.5f });
  INVARIANT(world.find(world.entities[handle]) == handle);
  return world.entities[handle];
}

// Release both flocks over the middle of the chunk. Everything about the demo that is not the
// world seed is fixed, so the same seed always produces the same first few seconds of flight.
static bool release_flocks(Game::World& world, u64 seed)
{
  Handle<Game::Flock> const starlings = world.content.find<Game::Flock>("starlings");
  Handle<Game::Flock> const gulls     = world.content.find<Game::Flock>("gulls");
  if (!starlings || !gulls) return false;

  constexpr f32        inset = Game::chunk_size * 0.25f;
  constexpr Game::Area nursery{
    .min = { inset, inset },
    .max = { Game::chunk_size - inset, Game::chunk_size - inset },
  };
  Game::spawn_flock(world, starlings, starling_count, seed, nursery);
  Game::spawn_flock(world, gulls, gull_count, seed, nursery);
  return true;
}

Runtime* start(int /*argc*/, char* /*argv*/[])
{
  auto state = std::make_unique<Runtime>();
  state->clock.set_rate(32);

  u64 const seed = random_seed();

  Game::Entity const player = spawn_player(state->world);
  if (auto result = state->app.load(state->world, player); !result) {
    std::println("Failed to open application: {}", result.error());
    return nullptr;
  }
  generate_environment(state->world, seed);
  if (!release_flocks(state->world, seed)) {
    std::println("Failed to find the demo's flocks in content/boids.lua.");
    return nullptr;
  }
  return state.release();
}

static void step(Runtime& state, i64 /*tick*/)
{
  state.app.step(state.world, state.cmds);
  state.cmds.dispatch(state.world);
  state.world.advance();
  // Simulation systems run on the state the commands left behind, and on the fixed timestep: the
  // flock is reproducible only because it never sees a variable frame time.
  state.flocking.step(state.world, state.clock.period());
}

static void update(Runtime& state, f32 delta) { state.app.update(state.world, delta); }

static void render(Runtime& state, f32 alpha) { state.app.render(state.world, alpha); }

void iterate(Runtime& state)
{
  auto tick = state.clock.tick();
  for (auto steps = state.clock.advance(); steps > 0; steps--)
    step(state, tick++);
  DEBUG_ASSERT(tick == state.clock.tick());

  update(state, state.clock.delta());

  render(state, state.clock.alpha());
}

void handle_event(Runtime& state, SDL_Event const& event) { state.app.handle_event(event); }

void quit(Runtime* state) { delete state; }
