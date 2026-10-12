# Maintenance and build guide

## Known hardware

NM-TV-154, classic ESP32-D0WD-V3, 4 MiB flash, no PSRAM, ST7789 240×240 display, CH340 USB serial. Confirm the board and COM port before flashing.

| Function | Pin / setting |
| --- | --- |
| HSPI clock / MOSI | GPIO 14 / 13 |
| Display CS / DC | GPIO 15 / 2 |
| Display reset | Not wired |
| LCD power / backlight | GPIO 21 LOW / 19 LOW |
| Capacitive touch | GPIO 32 / T9 |
| Display | Rotation 0, BGR order, inversion enabled, CGRAM offset |
| CPU / flash | 240 MHz, DIO at 40 MHz, 4 MiB |

The display uses a 28,800-byte packed 4-bit framebuffer. Artwork is generated from `art/build_avatars.py`; commit regenerated `AvatarArt.h` and `DashboardPage.h` together.

## Install dependencies and compile

The tested versions are Arduino CLI 1.5.1, Arduino-ESP32 3.3.11, TFT_eSPI 2.5.43, and esptool 5.4.0. Python 3 runs the artwork generator; Node.js runs dashboard tests and the signing utility.

```powershell
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.11 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install TFT_eSPI@2.5.43
.\build.ps1 -Jobs 2
node tests/dashboard.cjs
```

Use `-ConfigFile` for a custom Arduino CLI configuration and `-BuildPath` for an alternate build cache. Keep the two-job default on resource-constrained PCs. Outputs go to the ignored `firmware/` directory. The build script forces `DisplayConfig.h` into C++ compilation and selects `min_spiffs` with PSRAM disabled.

Source hashes in `source-sha256-0.5.0.json` identify the exact 21 firmware files imported from the tested release. These are source hashes, not a promise of bit-for-bit identical builds on every workstation.

## Signed updates

`DeskBuddy/ReleasePublicKey.h` is safe to publish. Its matching private signing key is held outside this repository and is needed for updates accepted by already-installed units.

```powershell
node sign-update.cjs firmware/DeskBuddy.ino.bin 0.6.0 PATH-TO-PRIVATE-KEY firmware/companion-0.6.0.buddy
```

Use the version of the firmware actually being signed. Never commit the private key. A contributor can compile without it, but cannot sign an update accepted by these units. Key replacement requires a deliberate trust migration. The package magic remains `DBFW030`; it is a format identifier, not the current release version.

## Flash layout and recovery

| Region | Offset | Size |
| --- | --- | --- |
| NVS | 0x9000 | 0x5000 |
| OTA metadata | 0xe000 | 0x2000 |
| app0 | 0x10000 | 0x1e0000 |
| app1 | 0x1f0000 | 0x1e0000 |

Prefer signed wireless updates once the `min_spiffs` layout is installed. A merged image at 0x0 is for first installation on matching hardware and replaces settings. After wireless updates, determine the running and selected slot before USB recovery; do not assume app0 is active. Version 0.2.0 requires a one-time partition migration with NVS preserved. No eFuse changes are needed.

The historical 0.5.0 [prototype reference](prototype-reference.txt) contains serial diagnostics and earlier installation notes. Use the current field guide for the new dashboard behavior and display color order. USB diagnostic `d` exposes a temporary development session; do not publish its output.

## Before a release

1. Build with the pinned dependencies and run dashboard checks.
2. Review generated artwork and verify both animation frames on hardware, including transparent pixels.
3. Test saved settings migration, clock, weather, touch, and Wi-Fi signal.
4. Sign with the trusted key and test installation/startup confirmation and recovery behavior appropriate to the change.
5. Package from build outputs, never a configured flash dump. Verify public NVS is blank and no private key or credentials are included.
6. Record hashes, test scope, and remaining limitations; update the field guide and version notes.

The repository excludes compiled outputs and the previous local test logs. [0.5.0 validation](validation-0.5.0.md) summarizes the completed hardware checks.

## 0.6.0 state and diagnostics

Settings use `appcfg6` and migrate separately from previous versions. The five-page array and new avatar/color preferences are copied and validated before writing. Old records are retained for downgrade. Forecast cache uses `weather6`; the first upgraded boot fetches a new seven-day response.

The local dashboard server and the on-screen access card have independent lifetimes. Successful unlock returns to normal rendering. Authenticated remote and keepalive POSTs refresh the ten-minute inactivity timer; status GETs do not. Visible browser tabs send keepalive every 25 seconds. Firmware uploads retain the dedicated update screen and block remote navigation.

`/api/control` accepts `next`, `previous`, `home`, `page` (with an enabled app ID), `pause`, and `resume`. All require the existing session token. `/api/state` reports active page/avatar, pause state, whether the access screen is visible, and the number of forecast days. No frame capture of the private access screen is allowed.

Weather daily dates are formatted with the response's `utc_offset_seconds`, not the clock setting. See the [Open-Meteo API time and daily-variable definitions](https://open-meteo.com/en/docs). Current data remains usable if a seven-day payload is incomplete; the forecast shows an unavailable state instead of invented values.
