# 0.7.0 validation

## Changes

Neon Arcade artwork page (ID 5), six-page navigation, a draft-only Bandicuss preset, neutral device wording, explicit v0.6 settings migration, and a clean gift settings generator. Companion remains enabled by the preset. The original owner's unit is not flashed.

## Verified before installation

- Dashboard tests pass for all six pages, preset draft behavior, explicit Save, and preservation of avatar/mode/colors/location/time/units. No Wi-Fi credentials are submitted by the preset.
- The 240×240 scene preview has been inspected for composition and sign readability.
- The second unit identifies as NM-TV-154 with ESP32-D0WD-V3 and 4 MiB flash; its hardware identity differs from the original unit.

## Device checks

The build passed: 1,412,761 program bytes (71% of the 1,966,080-byte slot), 55,572 bytes of static RAM. All 25 compiler source files match the repository. The signed package was generated with the existing trusted release key.

The second unit was flashed successfully. Startup checks passed. A synthetic v0.6 preference record was migrated in actual flash: name, location, clock, avatar, color profile, page order and rotation were preserved; Neon Arcade stayed disabled and the old record remained available. The clean gift settings were then written and verified byte-for-byte.

Offline artwork and all six page transitions passed, including Home returning to Neon Arcade. An actual framebuffer capture was reviewed for clipping and sign readability.

Physical feedback, network checks, and the 30-minute run remain pending; this document will be updated with their actual results.
