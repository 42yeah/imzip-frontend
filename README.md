# imzip-frontend

This is `imzip`, an (almost) WASM-only frontend capable of batch compressing images.

It makes use of [Sokol](https://github.com/floooh/sokol), [ImGui](https://github.com/ocornut/imgui), and [miniz](https://github.com/richgel999/miniz).

## Build:

```bash
> mkdir build
> cd build

> emcmake cmake -G Ninja ..
> emcmake cmake --build .
```

To build a Release version:

```bash
> emcmake cmake -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel ..
> emcmake cmake --build .
```

The project is not compatible with Windows, macOS and Linux since as of right now, it heavily depends on Emscripten APIs.

## Run:

```bash
> emrun imzip.html
```

Enjoy!

![Logo](logo.png)
