# Agent Guide: SLMP C++ Minimal

This repository is part of the PLC Communication Workspace and follows the global standards defined in `D:\PLC_COMM_PROJ\AGENTS.md`.

## 1. Project-Specific Context
- **Protocol**: SLMP (Seamless Message Protocol)
- **Authoritative Source**: Mitsubishi Electric specifications.
- **Language**: C++ (Minimal / Embedded focus)
- **Platform**: PlatformIO (Arduino/ESP32/etc.)
- **Role**: Lightweight Communication Library for embedded devices.

## 2. Mandatory Rules (Global Standards)
- **Language**: All code, comments, and documentation MUST be in **English**.
- **Encoding**: Use **UTF-8 (without BOM)** for all files to prevent Mojibake.
- **Mandatory Static Analysis**:
  - All changes must pass `cppcheck` or `clang-tidy`.
  - Use PlatformIO linting tools to verify compliance.
- **Documentation Structure**: Follow the Modern Documentation Policy:
  - `docsrc/user/`: User manuals and API guides. [DIST]
  - `docsrc/maintainer/`: Protocol specs and internal logic. [REPO]
  - `docsrc/validation/`: Hardware QA reports and bug analysis. [REPO]
- **Distribution Control**: Ensure `library.json` or `library.properties` excludes `docsrc/maintainer/`, `docsrc/validation/`, and `TODO.md` from library packages.

## 3. Reference Materials
- **Official Specs**: Refer to `local_folder/CLPA_DOWNLOAD_.../` for authoritative English manuals (Local only).
- **Evidence**: Check `docsrc/validation/reports/` for verified communication results with Mitsubishi PLCs.

## 4. Development Workflow
- **Issue Tracking**: Log remaining tasks in `TODO.md`.
- **Change Tracking**: Update `CHANGELOG.md` for every fix or feature.
- **QA Requirement**: Every hardware-related fix must include an evidence report in `docsrc/validation/reports/`.

## 5. API Naming Policy

Detailed naming policy lives in `docsrc/maintainer/API_UNIFICATION_POLICY.md`.

Public API rules:

- The canonical protocol client class name is `slmp::SlmpClient`.
- If a separate string-address facade is ever added, reserve `slmp::SlmpDeviceClient` for that layer instead of renaming the protocol client.
- Keep the explicit embedded API: `readWords`, `writeWords`, `readRandom`, `readBlock`, and related typed names.
- Keep 32-bit helpers explicit as `readDWords`, `writeDWords`, `readOneDWord`, `writeOneDWord`, `readFloat32s`, `writeFloat32s`, `readOneFloat32`, and `writeOneFloat32`.
- Keep async state-machine naming as `begin` + sync method name, plus `update` and `isBusy`.
- Do not add a generic `read()` or `write()` facade.

Transport naming rules:

- Transport helper names must include the transport kind, for example `ArduinoClientTransport` and `ArduinoUdpTransport`.
- Optional UDP transport must remain separately gateable so TCP-only builds avoid unnecessary code and includes.
- 32-bit codec helpers should include both type and word order, for example `packUInt32LowWordFirst` or `unpackFloat32LowWordFirst`.

