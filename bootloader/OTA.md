# Flashing Firmware via BLE OTA (wch-ota-tool)

This guide covers using [`wch-ota-tool`](https://github.com/luckyPtr/wch-ota-tool) to send new firmware to a badge over BLE, using the direct-xip ping-pong scheme (see main PR description / `bootloader/README.md` for the underlying design).

## 1. Download and extract the toolchain

Download the tool from the [luckyPtr/wch-ota-tool GitHub repo](https://github.com/luckyPtr/wch-ota-tool). Extract the zip — you'll get a folder like this:

<img width="793" height="40" alt="image" src="https://github.com/user-attachments/assets/b7f64663-e3c9-4f05-b821-a2cc3ee42fa5" />

Inside, you'll find a subfolder containing `wch-ota-tool.exe` alongside its DLL dependencies (`QtsCore.dll`, `WCHBLEDLL.dll`):

<img width="996" height="339" alt="image" src="https://github.com/user-attachments/assets/52cbac1c-f099-4b89-adc1-c2eb3201cd5d" />


## 2. Open a terminal in that folder

Navigate into the subfolder containing `wch-ota-tool.exe` in File Explorer, then type `cmd` in the address bar and press Enter — this opens a terminal already pointed at the right directory.

<img width="928" height="347" alt="image" src="https://github.com/user-attachments/assets/a0673a0d-8d25-4599-b356-fba435419814" />


This opens a Command Prompt window at that location:

<img width="1474" height="749" alt="image" src="https://github.com/user-attachments/assets/adad355d-15ac-42fa-966f-434046c2692d" />


## 3. Put the badge into BT pairing mode

Before sending anything, make sure the badge is advertising and in BT pairing mode (via the badge's own menu — BT-PAIRING option).

## 4. Determine which slot to flash

**Before every OTA update**, check which slot is currently active by reading characteristic `0xFEE2` (service `0xFEE0`) via a BLE tool such as nRF Connect — a plain Read returns `0x01` (Slot A active) or `0x02` (Slot B active), no write needed.

**Always flash the binary matching the *inactive* slot.** Sending the wrong slot's binary can produce corrupted, unpredictable firmware behavior — see PR description for details. If Slot A is active, send the Slot-B-linked `.bin`; if Slot B is active, send the Slot-A-linked `.bin`.

## 5. Run the flash command
wch-ota-tool.exe -s --filter="LED Badge Magic" --timeout=2000 -c -e 0,54 -d "<path to your .bin file>" --addr=0x0 -r

Replace the path in `-d "..."` with the actual location of the correct slot's `.bin` file (built by this repo's CI — see the 8-variant build matrix for the right hardware/key/slot combination).

**Flag reference:**
- `-s` — scan for devices
- `--filter="LED Badge Magic"` — only connect to a device matching this name
- `--timeout=2000` — scan timeout in ms
- `-c` — connect
- `-e 0,54` — erase 54 blocks (4KB each = full 216KB slot) starting at relative offset 0
- `-d "<path>"` — the firmware binary to send
- `--addr=0x0` — relative offset within the target slot (the firmware resolves this to the correct absolute address automatically based on which slot is inactive)
- `-r` — reset the badge after the transfer completes, triggering the slot switch

## 6. Watch the transfer

<img width="1919" height="903" alt="image" src="https://github.com/user-attachments/assets/4b4b29f0-ef6b-4dab-8bab-63916126a45f" />


Once it reaches 100%, the tool exits and the badge automatically resets and boots the new firmware from the slot you just wrote to.

## 7. Confirm the update

Reconnect via nRF Connect and read `0xFEE2` again — it should now report the opposite slot flag from before, confirming the badge is running the new firmware.
