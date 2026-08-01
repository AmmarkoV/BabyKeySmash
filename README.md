# BabyKeySmash

A fullscreen OpenGL toy for babies and toddlers, in the spirit of
[tinyfingers.net](https://tinyfingers.net/): smash the keyboard and colorful
letters, balloons and dinosaurs pop on screen, wiggle the mouse to draw
sparkly trails, while the **webcam** and **microphone** feed live into
psychedelic shader effects. The window covers **all visible monitors** and is
locked down so random key mashing (Alt+F4, Alt+Tab, Super, ...) cannot close,
minimize or escape it — only a parent can quit.

See [PLAN.md](PLAN.md) for the design decisions behind the implementation.

## Features

- **Fullscreen kiosk window** spanning the whole X virtual screen (every
  monitor), created as an override-redirect GLX window with the keyboard and
  pointer grabbed — the window manager never sees any input, so there is
  nothing for a toddler to accidentally trigger.
- **Keyboard**: letters and digits pop as big colorful characters; every other
  key pops a random sprite from `textures/`.
- **Mouse**: movement leaves a soft glowing trail, clicks make sprite bursts.
- **Microphone** (ALSA): a capture thread computes an FFT and fills a texture
  in the ShaderToy audio layout (row 0 = spectrum, row 1 = waveform); the
  background plasma pulses with sound and a waveform ring dances around the
  mouse.
- **Webcam** (OpenCV): a mirrored live feed is stylized by a posterize +
  rainbow edge-glow shader and blended over the background.
- Missing webcam or microphone is fine — the corresponding effect is simply
  disabled.

## Quitting (parents only)

- Hold **Escape for 3 seconds**, or
- press **Ctrl+Shift+Q**, or
- type the word **`quit`** or **`closeapplication`** (works with Ctrl held).

Toddler mashing produces short, unordered keystrokes, so none of these fire
accidentally. As a last resort, `pkill babykeysmash` from another terminal or
SSH. *Known limitation*: VT switching (Ctrl+Alt+F1..F12) and Magic SysRq are
handled by the kernel/X server and cannot be blocked by an X client.

## Building

Dependencies (Debian/Ubuntu package names):

```
sudo apt install cmake g++ libx11-dev libglew-dev libopencv-dev libasound2-dev
```

Then:

```
cmake -S . -B build
cmake --build build -j$(nproc)
./build/babykeysmash
```

The executable can be started from any directory — if `shaders/` is not found
in the current directory it falls back to the source tree.

`opengl_depth_and_color_renderer/` contains the files reused verbatim from
the [RGBDAcquisition](https://github.com/AmmarkoV/RGBDAcquisition) renderer
(shader loading); the GLX context creation in `src/glx_window.c` was also
adapted from it.

## Customizing

### Textures

Every `*.png` in `textures/` is loaded at startup and popped on keypresses
(alpha channel respected, random bright tint applied). Just drop in more art.
A file whose name contains `trail` is reserved for the mouse trail. The
bundled placeholders were generated with:

```
python3 tools/make_textures.py     # needs python3-pil , python3-numpy
```

### Shaders

`shaders/background.frag` (iChannel0 = audio texture) and
`shaders/webcam.frag` (iChannel0 = webcam, iChannel1 = audio) are
**ShaderToy-compatible**: they are `mainImage(out vec4, in vec2)` bodies
compiled behind a preamble declaring `iResolution`, `iTime`, `iMouse`,
`iChannel0..3`. Any single-pass shader from
[shadertoy.com](https://www.shadertoy.com) can be pasted over these files
verbatim — e.g. webcam effects like
[cll3zf](https://www.shadertoy.com/view/cll3zf) or microphone visualizations
like [4sjfzm](https://www.shadertoy.com/view/4sjfzm).

### Start/exit hooks

If executable, `scripts/on_start.sh` runs at startup and `scripts/on_exit.sh`
on every exit path. The bundled scripts switch an RGB keyboard's lighting to a
wave effect while playing and restore the normal profile afterwards using
`polychromatic-cli` — edit them to match your hardware, or strip the execute
bit to disable them.

## Author

Ammar Qammaz (AmmarkoV)
