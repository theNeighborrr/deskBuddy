# DeskBuddy field guide

For firmware **0.6.0** on the NM-TV-154. DeskBuddy is the working product name; you can give your unit its own name.

## Start here

1. Plug in USB power. Leave the top untouched for two seconds while touch calibrates.
2. A new unit opens setup. On your phone, join the temporary Wi-Fi using the name and password shown on the device.
3. Stay connected if the phone reports “no internet.” Open `http://192.168.4.1`.
4. Enter your home **2.4 GHz** Wi-Fi details and tap **Connect my buddy**.
5. Wait for internet time to synchronize. Choose your weather city and save your settings.

The public firmware contains no saved home network or weather city. Enterprise Wi-Fi and networks requiring browser sign-in are unsupported.

## Controls

| Gesture | Action |
| --- | --- |
| Quick tap | Next enabled page |
| Hold about one second | First enabled page |
| Hold five seconds | Open phone dashboard |
| Tap with access screen visible | Return to the display; dashboard stays open |

## Open settings again

Hold the top for five seconds. With your phone on the same home Wi-Fi, open the address displayed on the device and enter the six-digit code. The address can change after a router restart. Use the temporary setup Wi-Fi option when the home connection is unavailable.

In the dashboard you can change the name (up to 18 basic characters), choose and reorder pages, adjust automatic page switching, select an avatar, set timezone and 12/24-hour time, and choose weather city and units. Tap **Save my settings**. Changing these preferences does not require entering your Wi-Fi password again.

Change networks in the Wi-Fi section. The new connection is tested before replacing the saved one. Tap **Done** when finished. Unlocking the dashboard returns the device to its normal pages. Keep the phone tab visible to keep the session open. After the tab is hidden or closed, the dashboard expires after ten minutes without activity. **Done** closes it immediately. Holding five seconds shows the access screen again.

## Remote control

Use **Previous**, **Home**, and **Next**, or **Jump to a page** to choose any enabled page. **Pause page rotation** holds the current page; **Resume** starts a fresh rotation interval. These controls apply immediately and do not change your saved page order. Touch controls keep working. Pause resets on device restart.

## Characters

Open **Make it yours → Companion avatar**, pick a character, and save. There is one Companion page. Under **When Companion appears**, choose **Keep my chosen avatar** or **Next avatar on each visit**. The chosen avatar is the starting point; rotation advances through all seven on each new landing, including touch, automatic page changes, and the remote. Animation frames do not count as new visits. Restart or saving settings starts from your chosen avatar again.

- **Weather Cloud:** cloudy, sunny, rainy, stormy, snowy, foggy, or nighttime appearance based on your selected city's weather.
- **Paperclip:** the office paperclip character.
- **Hacker Dolphin:** a dolphin with headset and terminal details.
- **Money Sun:** a sunny character with dollar-sign shades.
- **Mr. Meeseeks:** the blue character from Rick and Morty.
- **Pokémon Parade:** Pikachu, Bulbasaur, Charmander, and Squirtle walking together.
- **Joe Camel:** the sunglasses-wearing camel.

All seven animate on the device. Phone previews show one still frame. Weather Cloud starts neutral until conditions arrive; rain, storms, snow, and fog take precedence over its night appearance. Cached weather is labeled.

## Display colors

The corrected profile uses the panel's BGR color order: Pikachu is yellow, Charmander orange, Squirtle blue, and Joe Camel tan/orange. In **Display colors**, keep **Corrected colors (recommended)**. An alternate profile is available for panels with the opposite red/blue order. Save to apply.

## Understand the readings

**Clock:** Central, Eastern, Mountain, Pacific, Arizona, Hawaii, or UTC. The first four adjust for daylight saving. After synchronization it keeps time during a network outage, but power loss or restart requires internet time again; there is no battery-backed clock.

**Weather:** refreshes about every fifteen minutes online. A saved/offline or old reading is not a new observation. Weather data comes from [Open-Meteo](https://open-meteo.com/). This is not an emergency alert service.

**This Week:** seven days of weather icons, low/high temperatures with range bars, and precipitation chance. The date under the rows identifies the beginning of the forecast in the selected city’s timezone, independent of the clock timezone. `--` means no precipitation-probability value is available. Saved/old forecasts are labeled; the display does not relabel an old first row as today.

**Wi-Fi Signal:** measures the connection to the router, not download speed. Device readings refresh once per second; the phone readout refreshes about every five seconds. Offline readings clear.

| Quality | Approximate signal |
| --- | --- |
| Excellent | -55 dBm or stronger |
| Good | -67 through below -55 dBm |
| Fair | -75 through below -67 dBm |
| Weak | Below -75 dBm |

Less-negative values indicate a stronger received signal. The labels do not guarantee a working internet connection.

## Install an update

1. Obtain a signed `.buddy` package for this hardware and its trusted release key.
2. Open the phone dashboard and expand **Wireless updates**.
3. Choose the package, tap **Install update and restart**, and leave USB power connected.
4. Wait for restart and confirmation. Reopen the dashboard to check the version and settings.

Uploads write to the spare firmware slot. Rejected or interrupted uploads retain the running version. A new build must pass startup checks before confirmation. **Restore previous firmware** selects the available previous image and restarts.

Versions 0.3.0, 0.4.0, and 0.5.0 can update to 0.6.0 wirelessly. Version 0.2.0 needs a USB partition migration first. Each version keeps its own application preferences; Wi-Fi is shared. The first upgrade to 0.6.0 preserves your preferences and adds This Week after your existing enabled pages. Upgrading from 0.5.0 preserves the chosen avatar and starts in fixed-avatar mode. Earlier versions keep their own preference records for recovery.

There is no automatic update server. A full merged USB image replaces firmware **and settings**; ask the maintainer to inspect the active slot before USB recovery.

## Troubleshooting

| Symptom | Try this |
| --- | --- |
| Taps do not register correctly | Restart and leave the top untouched for two seconds. |
| Dashboard will not open | Hold five seconds, use the currently displayed address, and put the phone on the same network. Guest-network isolation can prevent access; use the setup Wi-Fi fallback. |
| Phone leaves setup Wi-Fi | Choose to remain connected despite “no internet,” then open the numeric HTTP address. |
| Clock is waiting | Check that the 2.4 GHz network has internet access; a restart needs a fresh time sync. |
| Weather looks old | Check connectivity, city selection, and the saved/offline label. Allow the next refresh. |
| Avatar did not change | Select it under Companion avatar and tap Save my settings. Ensure Companion is enabled. |
| Firmware is rejected | Use the correct signed `.buddy` file. A raw `.bin` or file signed by another key is not a valid update. |
| Windows cannot see a USB port | Try a data-capable cable and another port; confirm the CH340 device appears. |

## Access and limits

No cloud account or companion server is needed. The temporary screen code opens a local dashboard session. Dashboard traffic uses HTTP and stored Wi-Fi credentials are not encrypted on this prototype. Use it on a trusted network. Keep screen codes, session tokens, and signing keys out of shared notes.

This version has five built-in pages. It does not yet support downloadable community apps, voice, cameras, or an AI chat service. Countdown is deferred and naming remains open. See the [roadmap](roadmap.md).
