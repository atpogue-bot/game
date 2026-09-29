#pragma once
#include "gfx/color.hh"
#include "gfx/rectangle.hh"

struct Texture;

// TODO: make this a user-interface implementation detail
constexpr f32 pixels_per_unit = 16.f;

struct Sprite
{
  Handle<Texture> atlas;
  Rectangle       source;
  Color           tint;
};
