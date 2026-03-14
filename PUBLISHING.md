# Publishing Notes

## Arduino Library Manager

This repository is structured as an Arduino library:

- `library.properties` is present at the repository root
- public headers live under `src/`
- installable example sketches live under `examples/`
- tagged releases provide a ready-to-install `.zip` archive

Recommended validation before submission:

```powershell
arduino-lint --compliance strict --library-manager submit .
```

If the library is not listed in Arduino Library Manager yet, direct users to the tagged release zip from GitHub Releases.

The CI workflow currently runs documentation link checks, host-side tests, size regression, and representative PlatformIO builds. Keep `arduino-lint --library-manager submit` as a manual gate until you are ready to apply for indexing.

## PlatformIO Registry

This repository also ships a `library.json` manifest for PlatformIO-compatible packaging.

No automatic `PlatformIO Registry` publish is configured in GitHub Actions. Keep publication manual until board-level validation is complete.

Validate the package locally with:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" pkg pack . --output $env:TEMP
```

Manual publish step after validation:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" pkg publish --owner <owner>
```

## GitHub Repository Metadata

These settings are managed on GitHub, not in the repository contents:

- About description
- Website URL
- Topics
- pinned release

Suggested description:

- `Compact SLMP 4E binary client library for Arduino-compatible ESP32 and RP2040 targets.`

Suggested topics:

- `arduino`
- `esp32`
- `rp2040`
- `platformio`
- `slmp`
- `melsec`
- `plc`

## Post-Release Habit

After each release:

1. Check the release page body and uploaded assets.
2. Download the release zip once and confirm it contains `src/`, `examples/`, and manifest files.
3. Add any post-release README or workflow fixes to `CHANGELOG.md` under `Unreleased`.
