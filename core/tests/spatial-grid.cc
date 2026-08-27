#include "core/spatial-grid.hh"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cmath>
#include <set>
#include <vector>

namespace { //////////////////////////////////////////////////////////////////////////////

  struct Vec2
  {
    f32 x = 0.f, y = 0.f;
  };

  static_assert(Point2<Vec2>);

  // The set of indices a query visits, so that order (an implementation detail) doesn't leak into
  // the expectations.
  std::set<u32> visited(SpatialGrid const& grid, Vec2 at, f32 radius)
  {
    std::set<u32> out;
    grid.each_near(at.x, at.y, radius, [&](u32 index) { out.insert(index); });
    return out;
  }

  // Every point genuinely within [radius] of [at]: what a query must not miss.
  std::set<u32> within(std::vector<Vec2> const& points, Vec2 at, f32 radius)
  {
    std::set<u32> out;
    for (u32 i = 0u; i < points.size(); ++i) {
      f32 const dx = points[i].x - at.x;
      f32 const dy = points[i].y - at.y;
      if (dx * dx + dy * dy <= radius * radius) out.insert(i);
    }
    return out;
  }

  void fill_with(SpatialGrid& grid, std::vector<Vec2> const& points)
  {
    grid.fill(u32(points.size()), [&](u32 index) { return points[index]; });
  }

  // A deterministic spread of points over [0, 32) x [0, 32), plus a few well outside it.
  std::vector<Vec2> scatter(u32 count)
  {
    std::vector<Vec2> points;
    points.reserve(count);
    u64 state = 0x9e3779b97f4a7c15ULL;
    for (u32 i = 0u; i < count; ++i) {
      state       = state * 6'364'136'223'846'793'005ULL + 1'442'695'040'888'963'407ULL;
      f32 const x = f32((state >> 33) % 3'200u) * 0.01f;
      state       = state * 6'364'136'223'846'793'005ULL + 1'442'695'040'888'963'407ULL;
      f32 const y = f32((state >> 33) % 3'200u) * 0.01f;
      points.push_back({ x, y });
    }
    return points;
  }

} ////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("spatial grid buckets points by cell", "[spatial-grid]")
{
  SpatialGrid grid;
  grid.reset(0.f, 0.f, 1.f, 4u, 4u);
  REQUIRE(grid.columns() == 4u);
  REQUIRE(grid.rows() == 4u);
  REQUIRE(grid.cell_size() == 1.f);
  REQUIRE(grid.size() == 0u);

  // One point in the middle of each of three distinct cells.
  std::vector<Vec2> const points = { { 0.5f, 0.5f }, { 2.5f, 0.5f }, { 2.5f, 3.5f } };
  fill_with(grid, points);

  REQUIRE(grid.size() == 3u);
  CHECK(grid.cell_of(0.5f, 0.5f) == 0u);
  CHECK(grid.cell_of(2.5f, 0.5f) == 2u);
  CHECK(grid.cell_of(2.5f, 3.5f) == 14u);

  REQUIRE(grid.cell(0u).size() == 1u);
  CHECK(grid.cell(0u)[0] == 0u);
  REQUIRE(grid.cell(2u).size() == 1u);
  CHECK(grid.cell(2u)[0] == 1u);
  REQUIRE(grid.cell(14u).size() == 1u);
  CHECK(grid.cell(14u)[0] == 2u);
  CHECK(grid.cell(1u).empty());
}

TEST_CASE("spatial grid keeps every point it is given", "[spatial-grid]")
{
  SpatialGrid grid;
  grid.reset(0.f, 0.f, 2.f, 8u, 8u);
  auto const points = scatter(512u);
  fill_with(grid, points);

  REQUIRE(grid.size() == 512u);

  size_t total = 0u;
  for (u32 cell = 0u; cell < grid.columns() * grid.rows(); ++cell) total += grid.cell(cell).size();
  CHECK(total == points.size());

  // Every index appears exactly once across all cells.
  std::set<u32> seen;
  for (u32 cell = 0u; cell < grid.columns() * grid.rows(); ++cell)
    for (u32 index : grid.cell(cell)) seen.insert(index);
  CHECK(seen.size() == points.size());
}

TEST_CASE("spatial grid queries find every true neighbour", "[spatial-grid]")
{
  SpatialGrid grid;
  f32 const   radius = GENERATE(0.f, 0.5f, 2.f, 9.f, 64.f);
  grid.reset(0.f, 0.f, 2.f, 16u, 16u);
  auto const points = scatter(400u);
  fill_with(grid, points);

  // Candidates are a superset of the neighbours: the grid may over-report (whole cells overlap the
  // query square) but must never miss a point that is genuinely in range.
  for (Vec2 const at : { Vec2{ 0.f, 0.f }, Vec2{ 16.f, 16.f }, Vec2{ 31.9f, 4.f } }) {
    auto const candidates = visited(grid, at, radius);
    auto const neighbours = within(points, at, radius);
    CHECK(std::ranges::includes(candidates, neighbours));
  }
}

TEST_CASE("spatial grid clamps points outside its region into the border", "[spatial-grid]")
{
  SpatialGrid grid;
  grid.reset(0.f, 0.f, 1.f, 4u, 4u);

  // Well below the origin, well past the far corner, and exactly on the far edge.
  std::vector<Vec2> const points = { { -100.f, -100.f }, { 100.f, 100.f }, { 4.f, 4.f } };
  fill_with(grid, points);

  CHECK(grid.cell_of(-100.f, -100.f) == 0u);
  CHECK(grid.cell_of(100.f, 100.f) == 15u);
  CHECK(grid.cell_of(4.f, 4.f) == 15u);

  // Clamped, not dropped: a query over the whole grid still sees all three.
  CHECK(visited(grid, { 2.f, 2.f }, 8.f).size() == 3u);
}

TEST_CASE("spatial grid is emptied by reset and replaced by refill", "[spatial-grid]")
{
  SpatialGrid grid;
  grid.reset(0.f, 0.f, 1.f, 4u, 4u);
  fill_with(grid, { { 0.5f, 0.5f }, { 1.5f, 1.5f } });
  REQUIRE(grid.size() == 2u);

  // A refill replaces the previous contents rather than appending to them.
  fill_with(grid, { { 3.5f, 3.5f } });
  CHECK(grid.size() == 1u);
  CHECK(grid.cell(0u).empty());
  REQUIRE(grid.cell(15u).size() == 1u);
  CHECK(grid.cell(15u)[0] == 0u);

  // A reset drops the contents and can reshape the covered region.
  grid.reset(-8.f, -8.f, 4.f, 2u, 2u);
  CHECK(grid.size() == 0u);
  CHECK(grid.columns() == 2u);
  CHECK(visited(grid, { 0.f, 0.f }, 16.f).empty());
  CHECK(grid.cell_of(-7.f, -7.f) == 0u);
  CHECK(grid.cell_of(-1.f, -1.f) == 3u);
}

TEST_CASE("spatial grid handles an empty fill", "[spatial-grid]")
{
  SpatialGrid grid;
  grid.reset(0.f, 0.f, 1.f, 3u, 3u);
  fill_with(grid, {});
  CHECK(grid.size() == 0u);
  CHECK(visited(grid, { 1.5f, 1.5f }, 4.f).empty());
}

TEST_CASE("spatial grid tolerates coincident points", "[spatial-grid]")
{
  SpatialGrid grid;
  grid.reset(0.f, 0.f, 1.f, 4u, 4u);
  std::vector<Vec2> const points(32u, Vec2{ 1.5f, 1.5f });
  fill_with(grid, points);

  REQUIRE(grid.size() == 32u);
  CHECK(grid.cell(grid.cell_of(1.5f, 1.5f)).size() == 32u);
  CHECK(visited(grid, { 1.5f, 1.5f }, 0.f).size() == 32u);
}
