-- Flocks for the boids demo.
--
-- The three weights below are the whole of the behaviour: separation keeps birds apart, alignment
-- makes them agree on a heading, cohesion pulls them back together. Raising cohesion above
-- separation packs the flock into a knot; raising separation above cohesion scatters it. The
-- interesting shapes live where they nearly cancel.
--
-- Distances are world units (one tile), speeds units per second, forces units per second squared.

-- The atlas is a 16x16 pixel tile every 17 pixels: one pixel of it is the gap to the next tile.
local function make_sprite(x, y, color)
  local tilesize, pitch = 16.0, 17.0
  return {
    atlas = 'content/kenney-1bitpack.png',
    source = { x * pitch, y * pitch, tilesize, tilesize },
    color = color
  }
end

-- Small and quick, turning hard enough to hold a sheet together at speed. Amber only so that
-- they read against the grass; a real starling is closer to the background than that.
flock 'starlings' {
  sprite = make_sprite(22, 10, 0xFFC15EFF),
  vision = 5.0,
  personal_space = 1.8,
  min_speed = 4.0,
  max_speed = 9.0,
  max_force = 26.0,
  separation = 2.6,
  alignment = 1.2,
  cohesion = 0.45,
  containment = 3.0,
  margin = 8.0,
}

-- Bigger birds that keep more room around them and turn less sharply, so they string out into
-- loose skeins instead of a sheet. Same four rules, different weights.
flock 'gulls' {
  sprite = make_sprite(20, 9, 0xE8E4D9FF),
  vision = 7.0,
  personal_space = 2.6,
  min_speed = 3.0,
  max_speed = 6.5,
  max_force = 11.0,
  separation = 1.8,
  alignment = 0.8,
  cohesion = 0.5,
  containment = 3.0,
  margin = 10.0,
}
