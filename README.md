# pacui
A small Qt Quick graphical frontend for the Arch Linux `pacman` package manager. It follows the Omarchy theme and language, and runs privileged operations through `pkexec`.

This app is vibecoded.

## What it does

- Search the repositories with `pacman -Ss`
- Show repository, version, description and whether a package is installed
- Install and remove packages with `pkexec pacman`
- Upgrade the system with `pkexec pacman -Syu`
- Show command output in a log pane

Actions that change the system use `--noconfirm`; the button you press is the confirmation.

## Build and run

Requires `qmake6`, `make`, a C++ compiler, Qt 6 base/declarative/wayland, `pacman` and `polkit`.

```sh
./bin/build
./build/pacui
```

## Test

```sh
./bin/test
```

## Package

```sh
./bin/install
```

The PKGBUILD builds for **x86_64** and aarch64, so you can install it here while testing. To produce a standalone **x86_64** release tarball:

```sh
./bin/release
```

That writes `build/pacui-0.1.0-x86_64.tar.gz`. On a non-x86_64 host, pass a cross qmake:

```sh
CROSS_QMAKE=x86_64-linux-gnu-qmake6 ./bin/release
```

## Shortcuts

| Key | Action |
| --- | --- |
| Enter / Space | Search |
| Ctrl+R | Refresh installed packages |
| Ctrl+U | Update system |
| ? | Toggle shortcut help |
| Q | Quit |

## Layout

- `src/main.cpp` — app setup, exposes the C++ objects to QML
- `src/backend.cpp` — package search and privileged commands
- `src/theme.cpp` — follows the Omarchy accent and Hyprland rounding
- `src/translator.cpp` — follows the Omarchy language
- `src/Main.qml` — the window
- `pkgbuild/` — x86_64 package, desktop entry and icon

Licensed under MIT.
