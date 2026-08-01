# BabyKeySmash — Plan

A fullscreen "keyboard smashing" toy for toddlers, in the spirit of
https://tinyfingers.net/ : every keystroke and mouse move produces colorful,
animated visuals; the webcam and microphone feed live into the background
effects; and the window is locked down so random key mashing (Alt+F4,
Alt+Tab, ...) cannot close or minimize it.

## Name

**BabyKeySmash** is kept as the working name. It is descriptive and it
usefully distinguishes the project from Scott Hanselman's Windows "BabySmash".
Alternative candidates if a rename is ever wanted: **KeyZoo** (animals pop per
key), **SmashCam** (emphasizes the webcam mode), **GigglePixels**. Nothing
below depends on the name except the executable / CMake project string.

## Decisions & assumptions (surfaced up front)

1. **Language/style**: C++ sources (`.cpp`, needed for the OpenCV C++ API) but
   written C-style: plain structs, no classes/templates/STL beyond what OpenCV
   forces, `fprintf` logging — matching the style of
   `opengl_depth_and_color_renderer`.
2. **Reused code**: `shader_loader.c` from
   `opengl_depth_and_color_renderer/src/Library/Rendering/ShaderPipeline/` is
   compiled in directly (with `-DUSE_GLEW=1`, wrapped in `extern "C"`).
   The GLX context creation in `src/glx_window.c` is adapted from
   `System/glx3.c` — a copy, not a link, because `glx3.c` hard-wires callbacks
   into `Scene/scene.h` and doesn't do fullscreen/input-grabbing.
3. **Multi-monitor**: the window is an **override-redirect** X window sized to
   the whole X virtual screen (`DisplayWidth/Height` of the root window), so it
   spans **all visible monitors**. Override-redirect also means the window
   manager never manages the window — Alt+F4 / Alt+Tab / Super have nothing to
   act on, which is most of the kiosk lockdown for free.
4. **Input lockdown**: in addition to override-redirect, the app calls
   `XGrabKeyboard` + `XGrabPointer` so *all* input is routed to it.
   *Known limitation*: Ctrl+Alt+F1..F12 (VT switch) and Magic SysRq are handled
   by the kernel/X server and cannot be blocked by an X client. Acceptable for
   a prototype; a dedicated kiosk X session can close that later.
5. **Parent exit combo**: hold **Escape for 3 continuous seconds** quits.
   Toddler key mashing produces short presses, so this is safe, and it needs
   no chording knowledge. (Ctrl+Shift+Q also quits, as a backup.)
6. **Microphone**: OpenCV cannot capture audio, so mic input uses **ALSA**
   (`libasound`, already installed). Capture thread → 1024-sample window →
   tiny radix-2 FFT (own ~50-line implementation) → a 512×2 texture in the
   exact **ShaderToy audio texture layout** (row 0 = spectrum, row 1 = raw
   waveform).
7. **ShaderToy compatibility**: shadertoy.com blocks scripted downloads
   (Cloudflare), so the referenced shaders (`cll3zf` webcam, `4sjfzm` mic)
   could not be vendored. Instead, every fragment effect is authored as a
   `mainImage(out vec4, in vec2)` body compiled behind a common preamble
   declaring `iResolution`, `iTime`, `iMouse`, `iChannel0`, `iChannel1`.
   Pasting those two shaders (or any single-pass ShaderToy shader) into
   `shaders/` later is then a copy-paste job.
8. **Textures**: no kid-art assets exist yet, so the prototype ships a few
   **procedurally generated placeholder PNGs** (balloon, star, heart, dino
   silhouette) created once by `tools/make_textures.py` into `textures/`.
   Any PNG dropped into `textures/` is picked up at startup (loaded with
   `cv::imread`, alpha respected). One-texture-per-key can come later by
   naming convention (`textures/key_a.png`), not needed for the prototype.
9. **Letters**: pressing a letter/digit shows that character big on screen
   (tinyfingers behavior). Rendered at runtime with `cv::putText` into a Mat →
   GL texture; no font-atlas machinery.
10. **No audio output** in the prototype (tinyfingers plays sounds; out of
    scope for now — noted as future work).
11. **Exit routes** (parent-only, toddler-safe): hold **Escape 3 s**,
    **Ctrl+Shift+Q**, or *type* the word **`quit`** or **`closeapplication`**
    (works with Ctrl held too, since matching is on keysyms).
12. **Hook scripts**: if executable, `scripts/on_start.sh` runs at startup and
    `scripts/on_exit.sh` at every exit path — used to switch the
    polychromatic keyboard lighting to `wave` while playing and restore the
    `reactive` profile afterwards.

## Dependencies

X11, GLX, OpenGL, GLEW, OpenCV 4 (webcam capture + image loading + text),
ALSA (mic), pthreads. All verified installed on this machine.

## Architecture

```
BabyKeySmash/
├── CMakeLists.txt          project + executable `babykeysmash`
├── PLAN.md
├── src/
│   ├── main.cpp            main loop, mode blending, sprite/letter spawning
│   ├── glx_window.c/.h     fullscreen-all-monitors GLX window (adapted glx3.c),
│   │                       override-redirect, XGrab*, event pump → plain C
│   │                       callbacks (key, mouse), Esc-hold exit timer
│   ├── shadertoy.c/.h      compile "mainImage" fragment files behind the
│   │                       ShaderToy preamble (uses shader_loader.c), set
│   │                       iTime/iResolution/iMouse/iChannelN, draw fullscreen quad
│   ├── sprites.cpp/.h      load textures/ PNGs (OpenCV), letter rasterization
│   │                       (cv::putText), pool of pop-scale-fade sprite instances
│   ├── webcam.cpp/.h       OpenCV VideoCapture thread → double-buffered RGB
│   │                       frame → GL texture (iChannel0 of webcam effect)
│   └── audio_alsa.c/.h     ALSA capture thread → FFT → 512×2 ShaderToy-layout
│                           audio texture (iChannel0 of mic effect)
├── shaders/
│   ├── background.frag     always-on colorful animated base (mic-reactive)
│   ├── webcam.frag         webcam effect (edge-glow / posterize style)
│   ├── sprite.vert/.frag   textured quad with pop animation + tint
│   └── (any ShaderToy paste-ins later)
├── textures/               PNGs; prototype placeholders generated by tools/
└── tools/make_textures.py  one-shot placeholder texture generator
```

Render order each frame: background shader (mic texture) → webcam shader
blended on top (mix factor slowly oscillates, so both are always "alive") →
sprites/letters with alpha blending → swap. Single GL context, capture
threads only touch CPU buffers; GL uploads happen on the main thread.

## Phases (each independently verifiable)

1. **Skeleton** — CMake + `glx_window` + clear-color animation.
   *Verify*: builds; window covers all monitors; Alt+F4/Alt+Tab do nothing;
   holding Esc 3 s (or Ctrl+Shift+Q) exits cleanly.
2. **ShaderToy plumbing** — fullscreen quad, preamble compilation,
   `background.frag` animated plasma responding to `iMouse`.
   *Verify*: animated colors follow mouse.
3. **Sprites & letters** — texture generation/loading, keypress → random
   sprite or the pressed letter pops at a random spot with scale/fade;
   mouse-move leaves a trail sprite at the cursor.
   *Verify*: mashing keys pops dinos/balloons/letters; moving mouse draws.
4. **Microphone** — ALSA thread + FFT texture; background pulses with sound.
   *Verify*: clapping/talking visibly changes the background.
5. **Webcam** — capture thread + effect shader blended in.
   *Verify*: camera image appears stylized and animates.
6. **Polish** — hide X cursor (kid sees sprite trail instead), keep
   screensaver away (`XResetScreenSaver` each minute), graceful behavior when
   webcam/mic are absent (effect simply disabled).

## Future work (explicitly not in prototype)

Sound effects on keypress, per-key texture mapping (`textures/key_<k>.png`),
vendoring the two referenced ShaderToy shaders once fetched manually in a
browser, mode-switch gestures, packaging/kiosk session setup.
