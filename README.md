# jfqt — Jellyfin client for Linux

Qt6 GUI wrapper for [jftui](https://github.com/Aanok/jftui), a Jellyfin TUI client that direct-plays media using mpv.

## Features

- Browse your Jellyfin libraries through a simple button-based interface
- Direct media playback via mpv (no transcoding)
- Resume-playback prompt support
- First-run setup wizard for server URL and credentials
- Automatic login with credential storage

## Dependencies

- Qt6 (Widgets)
- [jftui](https://github.com/Aanok/jftui) (must be on `PATH` at runtime)
- CMake 3.16+
- A C++17 compiler

## Building with CMake

```bash
cmake -B build
cmake --build build
```

Run the binary:

```bash
./build/jfqt
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

## Building with Nix

```bash
nix build
```

The binary is at `result/bin/jfqt`.

## Installation as a Nix flake

Add jfqt to your NixOS configuration or `home-manager` by referencing this flake.

In your `flake.nix` inputs:

```nix
inputs.jfqt.url = "github:tristanlirette/jfqt";
```

Then add the package to your environment:

```nix
environment.systemPackages = [
  inputs.jfqt.packages.${system}.default
];
```

Or with `home-manager`:

```nix
home.packages = [
  inputs.jfqt.packages.${system}.default
];
```

Run it directly without installing:

```bash
nix run github:tristanlirette/jfqt
```
