# WinPod

<p align="center">
  <img src="docs/demo.gif" alt="WinPod" width="256">
</p>

Just a small project: an AirPods battery popup for Windows, like the one on iPhone.

## How it works

AirPods keep broadcasting a BLE advertisement (Apple's "Proximity Pairing" message) with the model, battery of each bud and the case, charging, in-ear and lid state. WinPod listens for it with `BluetoothLEAdvertisementWatcher` and parses the bytes. I worked out the format mostly from this paper: https://inria.hal.science/hal-02394619/document

Connect/disconnect comes from Bluetooth Classic: the paired device list, filtered by Apple's vendor ID and the AirPods product IDs.

One catch: the advertisement only has battery in 10% steps, rounded down, so WinPod can show up to 9% less than your iPhone (91% shows as 90%). The exact value is in the encrypted part of the packet, which I can't read on Windows.

## Built with

- C++20, Win32, Direct3D 11, C++/WinRT for Bluetooth, Core Audio for the device list
- [Dear ImGui](https://github.com/ocornut/imgui) for the UI
- [trantor](https://github.com/an-tao/trantor), [spdlog](https://github.com/gabime/spdlog), [nlohmann/json](https://github.com/nlohmann/json), [stb_image](https://github.com/nothings/stb), [GoogleTest](https://github.com/google/googletest), all from [vcpkg](https://github.com/microsoft/vcpkg)
- Fonts: [Inter](https://github.com/rsms/inter) and [Font Awesome Free](https://fontawesome.com) (licenses in `resources/fonts/`)

## Building

You need Windows 11, Visual Studio 2026 with the C++ desktop workload, and CMake 3.25+.

```
git clone --recurse-submodules https://github.com/longsamaa/winpod.git
cd winpod
external\vcpkg\bootstrap-vcpkg.bat -disableMetrics
cmake -B build
cmake --build build --config Release
```

The exe ends up in `build/bin/Release/`, with `resources/` copied next to it. Logs go to `logs/app.log` beside the exe.

Tests: `ctest --test-dir build -C Debug` after a Debug build.

## Notes

Not affiliated with Apple. Switching the default device uses `IPolicyConfig`, an undocumented Windows interface, so a future Windows update could break that part.
