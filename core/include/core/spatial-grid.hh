#pragma once

// TODO: incremental updates. The grid is rebuilt from scratch by every `fill`, which is the right
// trade while every point moves every tick, but is wasteful for mostly-static sets.

#include "core/panic.hh"
#include "core/types.hh"
#include <concepts>
#include <span>
#include <vector>

// Anything with public `x` and `y` members, i.e. `glm::vec2`.
template <typename Type>
concept Point2 = requires (Type const& point) {
  { point.x } -> std::convertible_to<f32>;
  { point.y } -> std::convertible_to<f32>;
};

// A uniform partition of a rectangular region of 2D space. Buckets points into fixed size cells so
// that a neighbourhood query visits a bounded number of candidates instead of the whole set.
//
// Buckets share one dense array of indices with a per-cell offset into it (a compressed, "CSR"
// layout), so filling is a counting sort and a query walks contiguous memory.
//
// Points outside the covered region are clamped into the border cells rather than dropped: every
// point handed to `fill` is visited by a query that reaches the cell it landed in, which keeps
// callers honest about stray points instead of silently losing them.
struct SpatialGrid
{
  SpatialGrid() noexcept                         = default;
  SpatialGrid(SpatialGrid&&) noexcept            = default;
  SpatialGrid& operator=(SpatialGrid&&) noexcept = default;
  SpatialGrid& operator=(SpatialGrid const&)     = delete;
  ~SpatialGrid() noexcept                        = default;

  // Cover [columns] x [rows] cells of [cell_size] world units, starting at (x, y), and drop any
  // points already bucketed.
  void reset(f32 x, f32 y, f32 cell_size, u32 columns, u32 rows)
  {
    PRECONDITION(cell_size > 0.f, "cells must have a positive size");
    PRECONDITION(columns > 0u && rows > 0u, "grid must cover at least one cell");
    origin_x_  = x;
    origin_y_  = y;
    cell_size_ = cell_size;
    columns_   = columns;
    rows_      = rows;
    offsets_.assign(size_t{ columns_ } * rows_ + 1u, 0u);
    items_.clear();
    cells_.clear();
  }

  // Bucket [count] points, where `locate(i)` yields the position of the i-th point. The indices
  // handed back by `each_near` are these same i values.
  template <typename Locate>
  requires Point2<std::invoke_result_t<Locate, u32>>
  void fill(u32 count, Locate&& locate)
  {
    PRECONDITION(columns_ > 0u && rows_ > 0u, "filled a grid that covers no space");
    u32 const cells = columns_ * rows_;
    offsets_.assign(size_t{ cells } + 1u, 0u);
    items_.resize(count);
    cells_.resize(count);

    // Counting sort: tally each cell's population, turn the tallies into start offsets, then
    // scatter. The cell of every point is cached by the first pass so the scatter doesn't repeat
    // the (potentially non-trivial) `locate` call.
    for (u32 i = 0u; i < count; ++i) {
      auto const point = locate(i);
      u32 const  cell  = cell_of(f32(point.x), f32(point.y));
      cells_[i]        = cell;
      ++offsets_[cell + 1u];
    }
    for (u32 cell = 0u; cell < cells; ++cell) offsets_[cell + 1u] += offsets_[cell];
    INVARIANT(offsets_[cells] == count, "counting sort lost points");

    cursors_.assign(offsets_.begin(), offsets_.end() - 1);
    for (u32 i = 0u; i < count; ++i) items_[cursors_[cells_[i]]++] = i;
  }

  // Visit the index of every point bucketed in a cell that overlaps the square of half-width
  // [radius] centred on (x, y). These are candidates, not neighbours: cells are coarser than the
  // query, so the caller still applies the exact distance test.
  template <typename Visit>
  requires std::invocable<Visit, u32>
  void each_near(f32 x, f32 y, f32 radius, Visit&& visit) const
  {
    PRECONDITION(radius >= 0.f, "queried a negative radius");
    if (columns_ == 0u || rows_ == 0u) return;
    u32 const first_column = column_of(x - radius);
    u32 const last_column  = column_of(x + radius);
    u32 const first_row    = row_of(y - radius);
    u32 const last_row     = row_of(y + radius);
    for (u32 row = first_row; row <= last_row; ++row) {
      u32 const start = row * columns_;
      for (u32 column = first_column; column <= last_column; ++column) {
        u32 const cell = start + column;
        DEBUG_ASSERT(cell + 1u < offsets_.size());
        for (u32 i = offsets_[cell]; i < offsets_[cell + 1u]; ++i) visit(items_[i]);
      }
    }
  }

  // The cell a coordinate falls in, clamped to the covered region.
  [[nodiscard]] u32 cell_of(f32 x, f32 y) const noexcept
  {
    return row_of(y) * columns_ + column_of(x);
  }

  // The indices bucketed in one cell, in ascending order.
  [[nodiscard]] std::span<u32 const> cell(u32 index) const noexcept
  {
    DEBUG_ASSERT(index < columns_ * rows_);
    return { items_.data() + offsets_[index], offsets_[index + 1u] - offsets_[index] };
  }

  [[nodiscard]] u32 size() const noexcept { return items_.size(); }

  [[nodiscard]] u32 columns() const noexcept { return columns_; }

  [[nodiscard]] u32 rows() const noexcept { return rows_; }

  [[nodiscard]] f32 cell_size() const noexcept { return cell_size_; }

  // Explicit copy to prevent unintended and expensive implicit copies.
  [[nodiscard]] SpatialGrid copy() const { return *this; }

private:

  SpatialGrid(SpatialGrid const&) = default;

  // A coordinate outside the covered region clamps to the border; so does a NaN, which fails both
  // comparisons and lands in the first cell rather than in an out-of-bounds index.
  [[nodiscard]] u32 column_of(f32 x) const noexcept
  {
    f32 const cell = (x - origin_x_) / cell_size_;
    return cell > 0.f ? (cell < f32(columns_ - 1u) ? u32(cell) : columns_ - 1u) : 0u;
  }

  [[nodiscard]] u32 row_of(f32 y) const noexcept
  {
    f32 const cell = (y - origin_y_) / cell_size_;
    return cell > 0.f ? (cell < f32(rows_ - 1u) ? u32(cell) : rows_ - 1u) : 0u;
  }

  // Invariants (after `fill`):
  // - offsets_ is ascending, offsets_.size() equals columns_ * rows_ + 1
  // - items_ is a permutation of [0, offsets_.back())
  // - items_[offsets_[c] .. offsets_[c + 1]) are exactly the points bucketed in cell c

  f32 origin_x_  = 0.f;
  f32 origin_y_  = 0.f;
  f32 cell_size_ = 1.f;
  u32 columns_   = 0u;
  u32 rows_      = 0u;

  std::vector<u32> offsets_; // one start offset per cell, plus a tail equal to the point count
  std::vector<u32> items_;   // point indices, grouped by cell
  std::vector<u32> cells_;   // scratch: the cell each point landed in, cached between fill passes
  std::vector<u32> cursors_; // scratch: the next free slot in each cell during the scatter pass
};
