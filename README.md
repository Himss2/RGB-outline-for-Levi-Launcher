# OutlineRGB

A lightweight standalone native outline renderer for Minecraft Bedrock
on Android.

OutlineRGB is being developed as a standalone mod focused on providing
a customizable block/entity outline renderer with RGB colors and
adjustable outline thickness.

## Status

> ⚠️ Experimental / Work in Progress

The project is currently in the reverse-engineering and renderer
validation stage.

Current development stages:

- [x] Standalone project structure
- [x] ARM64 target
- [x] Native renderer architecture
- [x] Tessellator renderer abstraction
- [x] Target resolver architecture
- [ ] Verified outline hook
- [ ] Diagnostic ABI hook
- [ ] Basic red outline
- [ ] RGB animation
- [ ] Adjustable thickness
- [ ] Configuration system
- [ ] Release build

## Features

Planned:

- 3D block outline
- RGB animated outline
- Adjustable outline thickness
- Lightweight native implementation
- Standalone architecture
- Version-aware address/signature resolution

## Design

OutlineRGB does not directly depend on the complete ThickBaddie
or Flarial implementation.

Reverse engineering is used to identify the relevant Minecraft
rendering pipeline while keeping the final implementation minimal.

The intended pipeline is:

    Minecraft outline selection
            ↓
    target resolver
            ↓
    validated hook
            ↓
    ScreenContext
            ↓
    Tessellator
            ↓
    custom outline geometry
            ↓
    RenderMeshImmediately

## Current Target

The primary target is the native outline-selection rendering path.

The project intentionally does not hardcode previously unverified
addresses.

A candidate must be validated before it can be used by the hook.

## Build

The project currently targets Android ARM64.

### Xmake

```bash
xmake f -p android -a arm64-v8a
xmake
