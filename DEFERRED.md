# Deferred

Speculative or planned work that is deliberately not implemented yet. Anything small and local
enough to point at lives in a `// TODO:` next to the code instead.

## Flocking

- **Steering as a command.** Flocking is a system rather than a director because `Command` carries
  a single unit step and a batch is capped at 256, well under a flock's population. Once commands
  can express a continuous heading and a batch can hold a flock, a `FlockPilot` should generate
  them like any other director, and the boids demo becomes a test of the command pipeline instead
  of a bypass of it.
- **A general motion system.** `Flocking` integrates `Velocity` into `Pose` itself, because it is
  the only producer of `Velocity` today and needs the flock's own speed limits while doing it. When
  a second thing moves continuously, integration belongs in one system that owns every entity with
  both components, and flocking should stop at writing the steering force.
- **Predators.** A `Flock` should be able to name what its members flee from, so that the player
  (or a hawk) scatters the flock on approach. This is the cheapest large gain available to the
  demo: one more urge, weighted like the others.
- **Bounds from the world.** `Flocking` is constructed with the rectangle it keeps boids inside,
  which is the demo's one chunk. Real bounds belong to `World`, and a flock crossing a chunk
  boundary should be a loading question, not a steering one.
- **Component queries.** `Flocking::gather` walks every live entity once per flock kind because the
  registry cannot join stores. `BasicRegistry` already carries a TODO for `query<Ts...>()`; the
  gather pass is the first caller that would benefit.

## Content

- **Atlas tile pitch.** `content/kenney-1bitpack.png` lays out 16 pixel tiles on a 17 pixel pitch:
  the extra pixel is the gap between tiles. `content/boids.lua` samples it that way;
  `content/terrain.lua` samples on a 16 pixel pitch and so draws tiles that are one pixel out of
  register, growing worse across the atlas. Fixing the pitch there changes which sprite each
  terrain names, so the terrain coordinates have to be re-chosen at the same time.
- **Entity prefabs in Lua.** A flock definition doubles as a spawn recipe (a sprite plus the
  components its members need) purely because a flock is the only thing that spawns in bulk. What
  is really wanted is a prefab: a named set of components that any content can instantiate.

## Tests

- **A `game` test target.** The flocking system has no unit tests because `game/` has no test
  target, and adding one edits `game/CMakeLists.txt`. Worth adding: a flock is a good subject for
  characterisation tests -- that the order parameter rises, that the flock stays inside its bounds,
  and above all that a seed reproduces a run exactly.
