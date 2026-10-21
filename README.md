# WinPod

WinPod is a small Windows 11 tray app that shows AirPods status, similar to the popup on an iPhone or Mac. When AirPods connect, a toast appears in the bottom-right corner with the model name, an animation of the buds and case, and the battery level of the left bud, right bud and case.

Written in C++20 on Win32 + Direct3D 11 + Dear ImGui. Bluetooth data is read through C++/WinRT (`Windows.Devices.Bluetooth`).

## Features

- Reads the Apple Continuity "Proximity Pairing" BLE advertisement that AirPods broadcast: model, battery of each bud and the case, charging state, in-ear detection, case lid state.
- Detects AirPods connected over Bluetooth Classic by vendor/product ID, and tracks connect / disconnect.
- Toast UI: model name, sprite animations of the buds and case, battery rings with charging / in-ear / lid-open icons, slide-in animation.
- Tray icon with Open / Exit. The × button hides the toast; Exit quits the app.
- Lists audio input / output devices and changes the Windows default device from the toast.

Battery levels in the BLE advertisement are only sent in 10% steps; the exact 1% value is in an encrypted part of the packet, so WinPod shows 10% steps.

## Libraries

| Library | Used for | Source |
|---|---|---|
| [C++/WinRT](https://github.com/microsoft/cppwinrt) (Windows SDK) | Bluetooth LE advertisement watcher, device enumeration | Windows SDK |
| Win32, Direct3D 11, DXGI, DWM | Window, rendering, transparent rounded window | Windows SDK |
| Core Audio (MMDevice API) | Audio device list, device change notifications, default device | Windows SDK |
| [Dear ImGui](https://github.com/ocornut/imgui) | UI (Win32 + DX11 backends) | git submodule `external/imgui` |
| [trantor](https://github.com/an-tao/trantor) | Event loops (UI loop, Bluetooth worker thread) | vcpkg |
| [spdlog](https://github.com/gabime/spdlog) | Logging | vcpkg |
| [nlohmann/json](https://github.com/nlohmann/json) | Sprite sheet layout files | vcpkg |
| [stb_image](https://github.com/nothings/stb) | PNG decoding | vcpkg |
| [GoogleTest](https://github.com/google/googletest) | Unit tests | vcpkg |
| [vcpkg](https://github.com/microsoft/vcpkg) | Package manager (manifest mode, `vcpkg.json`) | git submodule `external/vcpkg` |

Fonts and icons in `resources/fonts/`:

- [Inter](https://github.com/rsms/inter) (Regular, SemiBold) — SIL Open Font License, see `LICENSE-Inter.txt`.
- [Font Awesome 7 Free](https://fontawesome.com) Solid — icons CC BY 4.0, font SIL OFL 1.1, see `LICENSE-FontAwesome.txt`.

## Building

Requirements: Windows 11, Visual Studio 2026 (MSVC, C++ desktop workload), CMake 3.25+, Git.

```
git clone --recurse-submodules https://github.com/longsamaa/winpod.git
cd winpod
external\vcpkg\bootstrap-vcpkg.bat -disableMetrics
cmake -B build
cmake --build build --config Release
```

If you cloned without `--recurse-submodules`, run `git submodule update --init` first.

The executable is `build/bin/<Config>/winpod.exe`; the build copies `resources/` next to it. Logs are written to `logs/app.log` next to the exe.

Run the tests:

```
cmake --build build --config Debug
ctest --test-dir build -C Debug
```

## Project layout

```
src/
  main.cpp          wWinMain, render loop
  app/              Application, Win32 Window, TrayIcon
  core/             AirpodParser, AirpodDevice
    ble/            BLE advertisement watcher
    classic/        Bluetooth Classic scanner
  audio/            Sound (MMDevice), NotificationClient, IPolicyConfig
  render/           IRenderer, d3d11/ backend
  resource/         ResourceManager (textures), stb_image
  ui/               ImGuiLayer, Toast, Table, ChooseProfile, FrameAnimation, components
  log/, utils/, platform/
resources/          sprite sheets (buds/, case/), fonts/, icons/
tests/              GoogleTest
```

## References

- Apple Continuity / BLE advertisement format: https://inria.hal.science/hal-02394619/document

## Disclaimer

WinPod is not affiliated with or endorsed by Apple. AirPods is a trademark of Apple Inc. Changing the default audio device uses `IPolicyConfig`, an undocumented Windows interface that may change in future Windows versions.
