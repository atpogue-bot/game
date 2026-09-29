```mermaid
---
config:
  look: handDrawn
  theme: neutral
  layout: elk
---
flowchart RL
	Core(Core)
  Platform(Platform)
  Game(Game)
  Graphics(Graphics Engine)
  App(Application)
	Platform --> Core
	Graphics --> Platform
	Game --> Core
	App --> Graphics
	App --> Game
```
