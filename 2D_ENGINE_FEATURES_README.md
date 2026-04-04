# Ume 2D Engine Expansion README

## Progress Update (2026-04-02) — VPak runtime integration

Completed in this session:
- **Engine/editor resource packing**: Editor toolbar icons and launcher icons are now compiled into `.vpak` files at build time via CMake post-build commands on `UmeEditor` and `UmeLauncher`.
  - `UmeEditor/Resources/Paks/EditorIcons.vpak` — all transport bar + content browser icons.
  - `UmeLauncher/Resources/Paks/LauncherIcons.vpak` — all launcher icons (22 textures).
- **`UmeVPackTool` extended** with `--source-dir <dir> --pack-name <name>` mode: packs any plain directory without needing a `.uproj` project file.
- **`ProcessUtils::LaunchProcessSync`** added — synchronous (blocking) process launch with exit-code return, used by the Play button.
- **`Texture2D::Create(path)` upgraded** to check `VPakRegistry` first (full path, then filename fallback) before falling back to the filesystem. No call sites needed to change.
- **Editor startup** mounts `EditorIcons.vpak` in `EditorLayer::OnAttach()` so all icon loads are resolved from the pack.
- **Launcher startup** mounts `LauncherIcons.vpak` in `LauncherLayer::OnAttach()`.
- **Play button** (`EditorLayer::OnScenePlay`) now runs `UmeVPackTool` synchronously against the active project before starting the scene, then mounts the resulting `Textures.vpak` via `VPakRegistry`.

## Progress Update (2026-04-01)

Completed in engine/editor codebase:
- Added first working Material System slice (render states on sprite components, inspector controls, scene serialization, and runtime application with sorting).
- Added a dedicated VPAK pipeline foundation:
  - New C++ tool target: `UmeVPackTool`.
  - Compiles texture assets into `Textures.vpak` plus `Textures.vpak.manifest.json`.
  - New automation script: `scripts/Pack-VPaks.ps1`.
  - Auto-copy outputs to runtime/editor and export-ready locations.
- Updated build workflow:
  - `build-and-run-editor.ps1` now supports `-PackVpaks` and `-ProjectFile`.
- Extended project configuration for future packaged media:
  - `RuntimeVPackDirectory` and `ExportVPackDirectory` added and serialized.

Current next step in progress:
- Complete section 1.1 with true shared Material assets (not only per-entity inline values), then continue strictly in roadmap order.

This document is a practical roadmap for turning Ume into a full-featured 2D game engine.

It is organized by systems, with concrete editor/runtime features to implement, why they matter, and what to build first.

## 1) Rendering and Materials

### 1.1 Material System
- Create a Material asset type.
- Support per-material parameters: textures, colors, floats, vectors, booleans.
- Add render states per material: blend mode, depth write/test, culling, sorting layer.
- Allow material overrides per entity and shared materials across entities.

### 1.2 Shader Workflow (Shader Maker)
- Add a Shader asset type with import, compile status, and error panel.
- Add hot reload for shader files while editor is running.
- Add a Shader Graph editor (node based) for non-programmer workflows.
- Graph output should generate shader code with preview.
- Support custom nodes and reusable subgraphs.
- Add material inspector UI for exposed shader properties.

### 1.3 Camera and Post Effects
- Add camera stack support (base + overlay cameras).
- Add post-process pipeline:
  - Bloom
  - Color grading (LUT)
  - Vignette
  - Chromatic aberration
  - Film grain
  - Pixelation
  - Distortion
  - Outline/highlight
- Add per-camera effect profiles and blending between profiles.
- Add global volume system (trigger areas that change camera effects).

### 1.4 Lighting and Shadows (2D)
- Add 2D lights: point, spot, directional-like.
- Add normal map support for sprites.
- Add shadow caster polygons and soft shadow quality settings.
- Add light layers and masks.

## 2) World Building and Grid Mode

### 2.1 Grid and Snapping
- Add editor grid overlays with custom cell size.
- Add snap controls: position, rotation, scale.
- Add snap presets (pixel perfect, tile, free).
- Add visual guides/rulers and alignment tools.

### 2.2 Tilemap and Block Placement
- Add Tile Palette window.
- Add Tilemap component with multi-layer support.
- Add paint, erase, fill, rectangle, line tools.
- Add autotiling rules and terrain transitions.
- Add collision generation from tile data.
- Add chunked tilemap rendering for performance.

### 2.3 Prefab Brush Mode
- Add placeable prefab brush for repeated objects (rocks, props, enemies).
- Add randomization options: rotation, scale, variant weights.
- Add scatter and stamp tools.

## 3) Animation and VFX

### 3.1 Sprite Animation
- Add animation clip assets from spritesheets.
- Add state machine editor (idle, run, jump, attack).
- Add blend parameters and transitions.
- Add animation events (footstep, hit frame, sound trigger).

### 3.2 Particle and VFX
- Add 2D particle system with modules:
  - Emission
  - Velocity over lifetime
  - Size/color over lifetime
  - Burst
  - Noise
- Support flipbook particles.
- Allow particle materials and shader graph integration.

## 4) Physics and Gameplay Tools

### 4.1 Physics Improvements
- Add collision layers/matrix editor.
- Add trigger filtering and callbacks.
- Add one-way platforms and slope helpers.
- Add physics debug view controls in editor.

### 4.2 Gameplay Components
- Add built-in movement modules (platformer, top-down).
- Add health, damage, and simple inventory templates.
- Add event bus/signals for decoupled gameplay logic.

## 5) Scripting and Runtime Systems

### 5.1 Script UX
- Add script creation wizard from templates.
- Add script compile diagnostics panel with click-to-file.
- Add script domain reload options (fast/clean reload).

### 5.2 Runtime Reflection and Serialization
- Expand serializable field types (arrays, dictionaries, enums, nested structs).
- Add custom inspector drawers for script fields.
- Add versioned serialization migration for assets/scenes.

## 6) UI and Tooling

### 6.1 UI Framework
- Add UI canvas system (screen-space/world-space).
- Add layout components (horizontal, vertical, grid, anchors).
- Add text styling, buttons, sliders, toggles, input fields.
- Add UI animation/tween integration.

### 6.2 Editor Quality of Life
- Multi-select in scene hierarchy and viewport.
- Better undo/redo coverage for all tools.
- Scene diff and prefab override visualization.
- Search/filter across components and assets.
- Command palette for common editor actions.

## 7) Asset Pipeline

### 7.1 Importers
- Add import settings for textures (filter, compression, sRGB, mipmaps).
- Add spritesheet slicer and pivot editing.
- Add audio importer settings (streaming, compression, loop metadata).

### 7.2 Asset Database
- Stable GUID-based references.
- Dependency graph and broken reference fixer.
- Background import queue with progress and errors.

## 8) Audio

### 8.1 Audio Engine Features
- Add mixer groups and buses.
- Add effects per bus (EQ, low-pass, reverb).
- Add spatial 2D audio with attenuation curves.
- Add snapshots for dynamic mix transitions.

## 9) Build, Packaging, and Distribution

### 9.1 Build Profiles
- Add build profiles per platform (desktop first).
- Add one-click export with project validation checks.
- Add asset bundle packing and compression options.

### 9.2 Modding and Workshop Preparation
- Define mod package format and sandbox rules.
- Add in-editor mod metadata and validator.
- Add upload/download workflow abstraction for platform integrations.

## 10) Performance and Diagnostics

### 10.1 Profiling
- CPU and GPU timeline profiler in editor.
- Frame debugger for draw calls and render passes.
- Memory profiler for textures, meshes, scripts, and scenes.

### 10.2 Optimization Features
- Sprite batching diagnostics.
- Culling debug overlays.
- ECS/system timings per frame.
- Optional fixed timestep visualization and warnings.

## 11) Testing and Stability

### 11.1 Automated Testing
- Unit tests for math, serialization, asset pipeline.
- Integration tests for scene load/save and script reload.
- Headless runtime smoke tests in CI.

### 11.2 Crash Recovery and Reliability
- Auto-save scenes.
- Recovery mode after crash.
- Better logging categories and filtering controls.

## 12) Suggested Build Order (Practical)

### Phase A (Core Tooling)
- Material system
- Shader assets + hot reload
- Grid/snapping improvements
- Tilemap MVP
- Better undo/redo and multi-select

### Phase B (Gameplay and Visuals)
- Post-process stack
- 2D lights and normal maps
- Animation state machine
- Particle system

### Phase C (Production Readiness)
- Asset database hardening
- Profilers and frame debugger
- Build/export pipeline
- Test automation and crash recovery

## 13) Minimum "Modern 2D Engine" Checklist

- Material and shader authoring pipeline
- Camera effects pipeline
- Tilemap/grid world building tools
- Animation graph and particle tools
- Reliable scripting workflow
- UI framework
- Asset import + database
- Profiling and diagnostics
- Build/export flow
- Test and recovery systems

---

If you use this roadmap as-is, start with Shader/Material + Grid/Tilemap first. Those two unlock the largest quality jump for both game visuals and editor workflow.