# icosa

A bouncing glenz vector over a checkerboard floor, inspired by the iconic
scene from [2nd Reality](https://en.wikipedia.org/wiki/Second_Reality)
(Future Crew, 1993).

The shape is a tetrakis hexahedron — a cube with a pyramid extruded from each
face — rendered as a braille-dot wireframe at ~30 fps. It bounces on a
perspective checkerboard with physics-based gravity, damping, and
squash-and-stretch deformation on impact.

Press any key to quit.

## Building from source

The only dependency is a C compiler and libc (with libm). No ncurses, no
external libraries.

```sh
git clone https://github.com/jesstelford/icosa.git
cd icosa
make
```

## Installing

### Any Linux (from source)

```sh
sudo make install
```

This installs to `/usr/local/bin` and `/usr/local/share/man/man1` by default.

For a user-local install (no root required):

```sh
make install PREFIX=~/.local
```

Make sure `~/.local/bin` is in your `PATH`.

### Arch Linux (AUR)

Using your preferred AUR helper:

```sh
paru -S icosa
```

Or manually with the included PKGBUILD:

```sh
cd pkg/arch
makepkg -si
```

### Debian / Ubuntu

```sh
sudo apt install build-essential
git clone https://github.com/jesstelford/icosa.git
cd icosa
make
sudo make install
```

### Fedora

```sh
sudo dnf install gcc make
git clone https://github.com/jesstelford/icosa.git
cd icosa
make
sudo make install
```

### Alpine

```sh
apk add build-base
git clone https://github.com/jesstelford/icosa.git
cd icosa
make
make install
```

## Uninstalling

```sh
sudo make uninstall
```

## Usage

Run it directly:

```sh
icosa
```

Or with a time limit:

```sh
timeout 3 icosa
```

### As a shell greeting

icosa works well as a terminal MOTD effect. For example, in Fish:

```fish
# ~/.config/fish/config.fish
if status --is-interactive
    timeout 3 icosa
end
```

Or in Bash/Zsh:

```sh
# ~/.bashrc or ~/.zshrc
timeout 3 icosa
```

## How it works

- **Geometry**: 14 vertices, 36 edges — 8 cube corners plus 6 pyramid tips
- **Rendering**: Each terminal cell maps to a 2×4 braille dot grid (Unicode
  U+2800–U+28FF), giving an effective resolution of 2·cols × 4·rows pixels.
  Edges are rasterized with Bresenham's line algorithm.
- **Floor**: Perspective checkerboard using 256-color background attributes
- **Physics**: Gravity, elastic bounce with damping, and squash-and-stretch
  deformation. Bounce restarts automatically when energy dissipates.
- **Input**: Terminal is set to raw mode; any keypress exits cleanly.
  SIGTERM (from `timeout`) is also handled.

## License

MIT
