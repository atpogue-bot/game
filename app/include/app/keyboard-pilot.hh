#pragma once
#include "app/pilot.hh"
#include "sys/keyboard.hh"
#include "sys/mouse.hh"

struct KeyboardPilot : Pilot
{
  void handle_event(SDL_Event const& event) override;
  void steer(World const&, CommandBuffer& out, Entity e) override;

private:

  Keyboard keyboard;
  Mouse    mouse;
};
