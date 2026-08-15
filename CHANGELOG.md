
---

# 2. `CHANGELOG.md`

Jangan membuat changelog seolah-olah fitur sudah bekerja.

```md
# Changelog

All notable changes to this project will be documented here.

## [Unreleased]

### Added

- Initial standalone OutlineRGB project structure.
- ARM64 native build target.
- Resolver abstraction.
- Renderer abstraction.
- AABB outline geometry abstraction.
- Initial Tessellator integration layer.

### Research

- Reverse-engineered ThickBaddie Outline scanner.
- Compared ThickBaddie rendering context usage with Flarial.
- Identified ScreenContext → Tessellator rendering path.
- Removed previously unverified hardcoded outline addresses from the
  implementation.

### Not Yet Working

- Final outline-selection hook.
- Red outline rendering.
- RGB animation.
- Adjustable thickness.
