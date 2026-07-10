# Publishing Notes

Audience: library maintainers and release work.

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

The default CI workflow intentionally stays host-only so normal development does not depend on PlatformIO package downloads. Keep PlatformIO sample builds and `arduino-lint --library-manager submit` as manual gates before registry or Arduino Library Manager publication.

## PlatformIO Registry

This repository also ships a `library.json` manifest for PlatformIO-compatible packaging.

The release workflow always builds and checks the PlatformIO package. Registry publication is disabled by default; a maintainer can explicitly enable the `publish_platformio` workflow-dispatch input after board-level validation. That path checks for an existing version and requires the `PLATFORMIO_AUTH_TOKEN` repository secret.

Validate the package locally with:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" pkg pack . --output $env:TEMP
```

Run the heavier PlatformIO sample gate only when you intentionally validate embedded examples or PlatformIO registry metadata:

```powershell
.\run_ci.bat --with-platformio
```

Manual fallback publish step after validation:

```powershell
$owner = "fa-yoshinobu"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" pkg publish --owner $owner
```

## GitHub Repository Metadata

These settings are managed on GitHub, not in the repository contents:

- About description
- Website URL
- Topics
- pinned release

Suggested description:

- `Compact SLMP binary client library for Arduino-compatible Wi-Fi and Ethernet targets.`

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
