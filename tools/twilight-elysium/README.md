# Twilight Elysium Engine

Standalone game engine for modern rendering, built separate from Halo CE.

## Structure

```
src/
  engine/       - Core runtime, memory, threading, input
  renderer/     - Render backend abstraction (Vulkan/Metal/D3D12/OpenGL)
  shader/       - Shader compiler, hot-reload, PBR pipeline
  asset/        - Asset loader, importers, converters
  pipeline/     - Frame graph, render passes, resource management
```

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Status

Scaffolded. No game-specific code yet.
