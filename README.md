# Flappy Bird for Q2 Pod

A standalone, SD-card-installable Flappy Bird-style game for the Shanling Q2 with [Q2 Pod custom firmware](https://github.com/DiamondBond/q2-pod). Inspired by [alxyng/flappybird](https://github.com/alxyng/flappybird), but implemented independently using the portable [Q2 Arcade game framework](https://github.com/lampsonlnguyen/q2-arcade).

## Features
- Single-button flapping, scrolling pipe pairs, collision detection, score and persisted best score.
- Fixed-step, allocation-free C gameplay using `Q2GameRunner` from Q2 Arcade.
- Native Q2 375x320 landscape framebuffer, procedural graphics (no SDL, artwork, or external assets).
- Centre button release to flap; Return pauses, Centre resumes, Return again exits; Centre restarts after collision. Wheel rotation and touchscreen signals are intentionally ignored to avoid accidental flaps.
- Native MIPS32 binary with SD-card game manifest, no firmware modification.
- GitHub Actions builds ZIP artifacts and publishes ZIP assets on version tags.

## Install
Download `q2-flappybird.zip` from a successful Actions build artifact (extract its outer Actions ZIP first) or a tagged GitHub release. Extract it to the microSD root so the hidden directory is `.flappybird/`, containing `q2app`, `q2-flappybird`, and `LICENSE.txt`. Reopen Library → Games to refresh. Requires Q2 Pod firmware supporting SD-card games.

## Controls

- **Centre click:** start, flap, and retry after a collision.
- **Return:** pause; Return again while paused to exit.
- **Centre while paused:** resume without an accidental flap.
- Click-wheel movement and touchscreen packets do not control the game.

## Build
Ubuntu/WSL: install `clang lld make gcc zip`, then `make check && make`. Output: `dist/q2-flappybird.zip`. For a direct GitHub release, push a version tag, e.g. `v1.0.0`.

## Credits
Mechanics and gameplay structure informed by alxyng's SDL Flappy Bird clone; no original code, sprites, or audio were copied. Q2 framework and platform adaptation from q2-arcade, GPL-3.0-or-later. See LICENSE.
