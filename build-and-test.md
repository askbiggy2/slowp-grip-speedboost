# Build and test checklist

- [ ] Install Android NDK and CMake/Ninja on a development computer.
- [ ] Obtain a compatible Android ARM64 Dobby build; do not use an arbitrary desktop library.
- [ ] Build `native/` for `arm64-v8a`.
- [ ] Package `libSlowpGripSpeedBoost.so` and `mod.json` as a QMOD archive.
- [ ] Install through QuestPatcher on the Slowp 0.99.144 installation.
- [ ] Capture `adb logcat` for tag `SlowpGripBoost` and confirm both hooks install.
- [ ] Test in a safe, low-risk in-game area: grip released baseline, grip held, release response.
- [ ] Confirm gravity remains normal and no persistent coefficient changes occur.
- [ ] If crashes occur, uninstall the QMOD and capture the crash log before changing hook signatures.

## ABI review update (2026-10-23)

The native hook typedefs now include the trailing IL2CPP `MethodInfo*` argument for both hooked managed methods. This is required for matching the usual IL2CPP native method ABI. A host-side C++ syntax-only check passed using a minimal Android log header stub; this does **not** verify ARM64 code generation, Dobby compatibility, hook signatures at runtime, or in-game behavior.

### Current build blocker in this workspace

This environment does not have an Android NDK/ARM64 Android compiler or a compatible Dobby library installed. Therefore the shared library cannot yet be built here. Do not install the current source on a headset until the native library is compiled and its method signatures are validated against the actual game binary.
