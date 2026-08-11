# Legend of Dragoon on Twilight Elysium

Goal: rebuild the original Legend of Dragoon experience using the Twilight Elysium engine with updated graphics and shaders, runnable on the local system and across the satalight grid.

## Runtime targets
- `local`: render/compute on the local machine
- `satalight`: distribute rendering/compute across quantum-loop nodes attached via space-edge-network

## Content scaffold
- Characters: Dart, Lavitz, Shana, Rose, Albert, Meru, Kongol, Miranda
- World/stage loading scaffold
- Turn-based combat scaffold
- Stage spawn points

## Graphics
- Modern shader pipeline via `te-shader`
- Frame graph via `te-pipeline`
- Renderer backend abstraction via `te-renderer`

## Next
1. Add real stage data from original game
2. Add character progression + menu systems
3. Add battle arena and camera system
4. Add shader assets + material system
