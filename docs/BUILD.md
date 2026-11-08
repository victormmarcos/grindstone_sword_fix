# Build and validation

The only build workflow for Grindstone Sword Fix. The version is set only on the
`project()` line of `CMakeLists.txt`; C++ and the Python tools read it from there.

## Requirements

- Windows x64, Visual Studio 2022 or newer with Desktop C++ tools.
- CMake 3.24+ and Ninja on PATH (a VS x64 developer shell has both).
- Python 3.11+ and PowerShell.
- vcpkg with `VCPKG_ROOT` set; dependencies follow `vcpkg.json`.

CommonLibSSE-NG 7.2.0 and OpenVR are pinned by URL and hash in `CMakeLists.txt`.
Don't hardcode local paths in source or presets.

## Compile

Build releases from a folder outside the user profile (e.g. `C:\GSFBuild`), so
no account name ends up in the DLL. Copy the project there (without `build/`)
and run, in a VS x64 developer shell:

```powershell
cmake --preset release
cmake --build --preset release --clean-first
ctest --preset release
```

CMake stages the DLL in `dist/Data/SKSE/Plugins/`. Release builds use path
mapping and ship no PDB.

## Validate and package

```powershell
python tools/CheckReleaseBinary.py dist/Data/SKSE/Plugins/GrindstoneSwordFix.dll
powershell -ExecutionPolicy Bypass -File tools/MakeInstallableZip.ps1
python tools/ValidateReleasePackage.py "Grindstone Sword Fix.zip"
```

The installer is `Grindstone Sword Fix.zip` and holds only
`SKSE/Plugins/GrindstoneSwordFix.dll` and `LICENSE.txt` (GPL-3.0, required
because CommonLibSSE-NG is GPL). Copy the staged DLL and the zip back to
the project folder, then delete the build folder.

## Limits

These checks don't prove the hook works in game. Test on a grindstone before
publishing.
