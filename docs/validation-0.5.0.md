# 0.5.0 validation

The source imported here matches the 21 firmware files used for the installed 0.5.0 build. No firmware behavior was changed during repository setup.

Completed on the NM-TV-154 prototype:

- Signed wireless upgrade from 0.4.0, startup checks, and version confirmation.
- Saved Wi-Fi, name, location, time preferences, page order, and rotation preserved; Weather Cloud selected on migration.
- All seven avatar selections saved; the highest avatar ID survived restart. Invalid avatar values were rejected without changing settings.
- Both frames of all seven characters matched device captures against artwork reference pixels, including transparency. Labels, captions, and full parade scene were checked for clipping.
- Seventeen weather-avatar mapping checks passed. Nighttime was the live-weather state exercised on the device; other states were reviewed in generated previews and logic checks.
- Clock, weather, signal page, touch controls, and automatic page rotation checked.
- 120-second stability run: 119 samples and no unexpected boots; observed free heap 87,984–153,780 bytes.
- Dashboard checks in a simulated DOM covered seven distinct previews, save behavior, signal/offline display, page controls, and safe city text rendering.
- Public packaged firmware was checked for blank NVS, matching signed and application images, private-key exclusion, and ZIP integrity.

The compiler reported 1,391,697 bytes of program use (70% of the 1,966,080-byte slot) and 55,396 bytes of globals. Short bench testing does not establish long-term reliability. Final owner feedback on the new character artwork remains open.

Earlier versions had additional update interruption, tamper, and rollback tests. Those historical checks were not all repeated for 0.5.0. The initial GitHub import contains source and documentation; the previously verified binary package remains local.
