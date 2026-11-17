# Flappy Bird for Q2 Pod

A standalone, SD-card-installable Flappy Bird-style game for the Shanling Q2 with [Q2 Pod custom firmware](https://github.com/DiamondBond/q2-pod). Inspired by [alxyng/flappybird](https://github.com/alxyng/flappybird), but implemented independently using the portable [Q2 Arcade game framework](https://github.com/lampsonlnguyen/q2-arcade).

## Features
- Single-button flapping, scrolling pipe pairs, collision detection, score and persisted best score.
- Fixed-step, allocation-free C gameplay using `Q2GameRunner` from Q2 Arcade.
- Native Q2 375x320 landscape framebuffer, procedural graphics (no SDL, artwork, or external assets).
- Centre button press (without waiting for release) or touchscreen tap to flap; Return pauses, Centre or tap resumes, Return again exits; Centre or tap restarts after collision. Wheel rotation is ignored.
- Native MIPS32 binary with SD-card game manifest, no firmware modification.
- GitHub Actions builds ZIP artifacts and publishes ZIP assets on version tags.

## Install
Download `q2-flappybird.zip` from a successful Actions build artifact (extract its outer Actions ZIP first) or a tagged GitHub release. Extract it to the microSD root so the hidden directory is `.flappybird/`, containing `q2app`, `q2-flappybird`, and `LICENSE.txt`. Reopen Library → Games to refresh. Requires Q2 Pod firmware supporting SD-card games.

## Controls

- **Centre click:** start, flap, and retry immediately on button down; key release does not flap again.
- **Tap screen:** start, flap, resume, or retry (on touch-down, as in Dino Runner).
- **Return:** pause; Return again while paused to exit.
- **Centre while paused:** resume without an accidental flap.
- Wheel rotation and touch movement do not trigger flaps. A touch-down flaps immediately. The first move is accepted if down was missed; release-only events are supported as a fallback without double-flapping.

## Diagnose controls on real Q2 hardware

Input recording is **off by default**. To capture the exact firmware messages Flappy Bird receives:

1. Install the newest build on the microSD card.
2. Create an **empty file** called `debug-input` inside `.flappybird/` on the microSD card (not `debug-input.txt`). On Windows PowerShell, from the SD card root: `New-Item -ItemType File .flappybird/debug-input`. On Linux/macOS: `touch .flappybird/debug-input`.
3. Launch **Flappy Bird** on the Q2. Press Centre a few times, tap the touchscreen in several locations, try holding and releasing your finger, then use Return twice to exit.
4. Eject the card and open `.flappybird/input.log`. Send this log file for analysis.
5. Delete `.flappybird/debug-input` after testing to disable logging.

Every log entry includes the monotonic timestamp (milliseconds), **raw firmware datagram**, whether the game accepted it (1/0), the game state (0 running, 1 paused, 2 over), and the bird's vertical position in fixed-point units. The log records up to 1,000 events per game launch and is overwritten on the next logged launch. A new log is only created when `debug-input` exists. Recording adds SD-card writes and can temporarily affect responsiveness; it is intended only for diagnosis.

## Build
Ubuntu/WSL: install `clang lld make gcc zip`, then `make check && make`. Output: `dist/q2-flappybird.zip`. For a direct GitHub release, push a version tag, e.g. `v1.0.0`.

## Credits
Mechanics and gameplay structure informed by alxyng's SDL Flappy Bird clone; no original code, sprites, or audio were copied. Q2 framework and platform adaptation from q2-arcade, GPL-3.0-or-later. See LICENSE.
