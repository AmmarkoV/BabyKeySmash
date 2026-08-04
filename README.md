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
- **Keyboard**: letters pop as big colorful characters; digits **count** —
  pressing 3 pops three of the same animal. Every other key pops a random
  sprite from `textures/`. With `--greek` the Greek alphabet pops instead:
  Latin keys are mapped through the Greek keyboard layout (a→Α, b→Β, g→Γ,
  ...) and native Greek layout keysyms are handled directly.
- **Spoken letters and numbers** with `--speech` (off by default): each
  letter and count is said aloud through espeak-ng — Greek letter names
  (άλφα, βήτα, ...) in `--greek` mode. Without it, keys just play sound
  effects.
- **Paired sounds**: when a popping sprite has a matching sound — texture
  `emoji_cow.png` and any `sounds/cow*.ogg` — that sound plays (the cow
  moos); otherwise a random clip is used.
- **Mouse**: movement leaves a soft glowing trail, clicks make sprite bursts,
  and the cursor can bat floating sprites around; sprites bounce off the
  screen edges.
- **Motion magic**: waving in front of the webcam sprinkles sparkles where
  the movement happened.
- **Calm mode**: `--simplebg` replaces the animated visuals with a quiet
  deep-blue-to-black gradient that breathes slowly, and leaves the webcam
  effect off — for winding down, a dark room, or a child who finds the busy
  effects too much. Sprites, letters and sounds keep working as usual.
  (`--background calm` gives the same gradient but keeps the webcam effect.)
- **Switchable effects**: backgrounds (plasma, sea, bubbles, calm), webcam effects
  (edge-glow, kaleidoscope, dot-mosaic, funhouse wobble) and sleepy scenes
  are discovered from `shaders/` at startup. They rotate every 3 minutes with
  a crossfade, a parent can switch them live with **Ctrl+Shift+B** /
  **Ctrl+Shift+W**, or one can be pinned with `--background sea`,
  `--webcam wobble`, `--sleepy moon`. `--list-shaders` prints every option.
- **Microphone** (ALSA): a capture thread computes an FFT and fills a texture
  in the ShaderToy audio layout (row 0 = spectrum, row 1 = waveform); the
  background plasma pulses with sound and a waveform ring dances around the
  mouse.
- **Webcam** (OpenCV): a mirrored live feed is stylized by a posterize +
  rainbow edge-glow shader and blended over the background.
- **Sounds**: every keypress triggers a random clip from `sounds/*.ogg`
  (decoded with libvorbisfile at startup, mixed in-process through ALSA —
  ogg keeps the repository ~15x smaller than wav). At most 2 clips play
  simultaneously and triggers are rate limited (0.35 s cooldown), so key
  smashing cannot spam audio. Populate/refresh the bank with
  `tools/import_sounds.sh [sourceDirectory]` (ffmpeg converts mp3s to the
  expected ogg format, trimmed to 4 s); delete any ogg you dislike.
- **Session flow**: `--volume 0..100` scales sounds and speech, `--voice f3`
  picks an espeak-ng voice variant (`f1..f5`, `m1..m7`, `croak`, `whisper`,
  implies `--speech`);
  `--minutes N` limits playtime — when time is up the screen crossfades into
  a calm night scene of twinkling stars and a **photograph of the real Moon,
  shaded with tonight's actual lunar phase** (so it matches the moon outside
  the window; faint earthshine keeps the dark limb visible, and
  `--moon-phase 0..1` forces a specific phase).
  Every session appends a line to `~/.babykeysmash_stats`
  (date, duration, keystrokes, mouse moves, clicks).
- Missing webcam, microphone, sounds or espeak-ng is fine — the corresponding
  feature is simply disabled (speech is off unless `--speech` is passed).

## Parent controls

Run `babykeysmash --help` for every option, `--list-shaders` for the
available effect and voice names. While playing, **Ctrl+Shift+B** switches
the background and **Ctrl+Shift+W** the webcam effect.

## Quitting (parents only)

- Hold **Escape for 3 seconds**, or
- press **Ctrl+Shift+Q**, or
- type the word **`quit`** or **`closeapplication`** (works with Ctrl held).

Toddler mashing produces short, unordered keystrokes, so none of these fire
accidentally. As a last resort, `pkill babykeysmash` from another terminal or
SSH. *Known limitation*: VT switching (Ctrl+Alt+F1..F12) and Magic SysRq are
handled by the kernel/X server and cannot be blocked by an X client.

## Building

Dependencies (Ubuntu 22.04 / 24.04 package names):

```
sudo apt install cmake g++ libx11-dev libglew-dev libopencv-dev libasound2-dev libvorbis-dev
```

Then:

```
cmake -S . -B build
cmake --build build -j$(nproc)
./build/babykeysmash
```

Or simply `./run.sh` (builds if needed, forwards arguments like `--greek`).
To install system-wide — binary to `/usr/bin/babykeysmash`, assets to
`/usr/share/babykeysmash` — run `./install.sh` (it installs the dependencies,
builds as your user and asks for sudo only when copying); remove everything again
with `./uninstall.sh`. See [INSTALL](INSTALL) for details.

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
A file whose name contains `trail` is reserved for the mouse trail; files
whose name contains `emoji` skip the random tint and keep their original
colors. The bundled art — hand-drawn placeholder shapes, the Greek alphabet
and ~64 kid-friendly emoji extracted from the system Noto Color Emoji font
(each glyph of that font is an embedded PNG) — was generated with:

```
python3 tools/make_textures.py     # needs python3-pil , python3-numpy , python3-fonttools
```

### Shaders

Every shader in `shaders/` is **ShaderToy-compatible**: a
`mainImage(out vec4, in vec2)` body compiled behind a preamble declaring
`iResolution`, `iTime`, `iMouse`, `iChannel0..3`. Any single-pass shader from
[shadertoy.com](https://www.shadertoy.com) can be pasted in verbatim — e.g.
webcam effects like [cll3zf](https://www.shadertoy.com/view/cll3zf) or
microphone visualizations like
[4sjfzm](https://www.shadertoy.com/view/4sjfzm).

**Adding one is just dropping in a file** — the filename prefix decides which
slot it joins and what its inputs are:

| filename | slot | iChannel0 | iChannel1 |
|---|---|---|---|
| `background_<name>.frag` | background | audio texture | — |
| `webcam_<name>.frag` | webcam effect | webcam image | audio texture |
| `sleepy_<name>.frag` | end-of-playtime scene | audio texture | — |

The audio texture follows the ShaderToy layout (row 0 = FFT spectrum,
row 1 = waveform). A new file appears in `--list-shaders`, joins the
rotation, and can be pinned by name — no code change, no rebuild.

### Hook scripts

If executable, these run at the matching moment — each is optional, strip the
execute bit to disable one:

| script | when |
|---|---|
| `scripts/on_start.sh` | at startup |
| `scripts/on_sleep.sh` | once when playtime ends (`--minutes N`) and the sleepy scene begins |
| `scripts/on_exit.sh` | on every exit path |

The bundled examples drive an RGB keyboard with `polychromatic-cli`: a wave
effect while playing, a dim slow breathing night colour when the sleepy scene
comes up, and the normal profile at full brightness again on exit. Edit them
to match your own hardware.

## Author

Ammar Qammaz (AmmarkoV)

## License

Source code is GPL-3.0 — see [LICENSE](LICENSE).

Bundled artwork keeps its own license and attribution, listed in
[CREDITS](CREDITS): the moon photograph is
[FullMoon2010.jpg](https://commons.wikimedia.org/wiki/File:FullMoon2010.jpg)
by Gregory H. Revera (CC BY-SA 3.0, cropped and downscaled), and the emoji
come from Google's Noto Color Emoji (SIL OFL 1.1). Note that the bundled
sound effects are of unverified origin — see CREDITS before publishing.
