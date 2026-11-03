# 0.6.0 validation

Installed on the NM-TV-154 prototype using a signed wireless update from 0.5.0. The previous 0.5.0 image remains available for recovery. The owner confirmed on the physical screen that Pikachu is yellow, Charmander orange, and Squirtle blue.

## Changes

- Correct NM-TV-154 panel color order from RGB to BGR; retain an alternate profile for differing panels.
- Current-weather artwork animates with conditions; wind, feels-like, and high/low readings remain visible.
- Add This Week with seven daily icons, temperature ranges, and precipitation probability.
- Add fixed or per-visit avatar selection with a tested navigation state machine.
- Keep the phone dashboard open alongside normal device rendering; add authenticated navigation and pause controls.
- Preserve 0.5.0 preferences and recovery records; add the weekly page on migration.

## Completed checks

Dashboard simulated-browser checks pass for all five pages, remote commands, visible-tab-only keepalive, rotation and color settings, and existing authentication/settings behavior.

Device startup checks passed for gestures, time/setup rules, navigation, avatar weather mapping, Wi-Fi signal, saved settings, and forecast parsing. The new build passed startup confirmation.

On-device checks passed for:

- Preserved name, Wi-Fi, chosen avatar, weather location, clock preferences, and page rotation; This Week appended to the selected pages.
- All seven avatars in per-visit sequence, including wraparound; selecting the already visible Companion page does not advance it.
- Avatar rotation mode surviving a restart.
- Authenticated Previous, Home, Next, direct page selection, and pause/resume following the saved order while the display remains active.
- Rejection of unauthenticated remote/keepalive requests, disabled pages, invalid controls, and invalid rotation/color settings without changing preferences.
- Live current weather and all seven forecast days; actual display captures reviewed for clipping and readability.
- Existing clock and signal pages retained.
- A 120-second run with the dashboard kept open, all five pages rotating, and per-visit avatars: 23 authenticated status samples, no unexpected restarts, and minimum sampled free heap of 79,664 bytes.

The final compiler input matches all 23 firmware source files. The compiler reports 1,402,661 program bytes (71% of the 1,966,080-byte slot) and 55,572 bytes of globals. Dashboard JavaScript tests also pass from the repository.

Forecast parser checks cover incomplete day arrays, unknown precipitation probabilities, and probabilities outside the valid range. Display captures confirm rendering and layout; they cannot verify the physical panel's red/blue order. Short bench checks do not establish long-term reliability.
