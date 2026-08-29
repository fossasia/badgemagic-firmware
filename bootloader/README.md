# IAP Bootloader (BackupUpgrade_JumpIAP)

This directory contains the IAP-side firmware source for reference and review.
It is **not built by this repo's Makefile or CI** — it's a separate MounRiver
Studio project, built independently.

## Building

1. Open `BackupUpgrade_JumpIAP` in MounRiver Studio.
2. Build → produces `BackupUpgrade_IAP.hex`.
3. JumpIAP (the fixed `j 0x6D000` stub) is a separate, smaller MounRiver
   project — build it the same way to produce `BackupUpgrade_JumpIAP.hex`.

## Factory provisioning (one-time, per new badge)

New badges need JumpIAP + IAP + one initial APP slot flashed once via wire/ISP
before they can receive any OTA updates. Use a hex-merge tool (e.g.
HexBinStudio) to combine three `.hex` files into one flashable image:

| # | File | Offset (0x) | Notes |
|---|------|-------------|-------|
| 1 | `BackupUpgrade_JumpIAP.hex` | `0` | Fixed stub, `j 0x6D000` |
| 2 | `badgemagic-ch582-slotA.hex` (from this repo's CI, matching hardware/key variant) | `1000` | Initial APP firmware — badges ship running Slot A |
| 3 | `BackupUpgrade_IAP.hex` | `6D000` | Direct-xip bootloader |

<img width="1345" height="650" alt="image" src="https://github.com/user-attachments/assets/a735c28c-c82c-4c3f-bc2a-caa16032caf4" />

Leave Slot B (`0x37000`–`0x6D000`) blank/erased — it stays empty until the
badge receives its first OTA update.

Flash the merged output once via wire/ISP (e.g. `wchisp flash merged.bin`).
From that point forward, all updates happen over BLE OTA — see the main PR
description and `src/ble/ota.h` for the direct-xip / ping-pong scheme this
enables.

## Updating IAP or JumpIAP itself

Neither is updatable via the current OTA protocol — OTA only ever writes to
APP slots A/B. If IAP or JumpIAP ever need to change, badges already in the
field require a new factory-flash pass (or a separate, not-yet-implemented
IAP-update mechanism) — this is a known limitation of the current design.
