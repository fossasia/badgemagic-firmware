### BLE Security (Optional PIN Authentication)

This firmware includes an optional PIN-based authentication mechanism for the
legacy profile. When enabled, the badge displays a 4-digit PIN on the LED matrix
upon entering BT-PAIRING mode. A client must send this PIN before any bitmap
transfer will be accepted.

Security is **disabled by default**, preserving full backward compatibility with
existing apps, libraries, and tools. It can be enabled via the badge's
**SECURITY** menu.

#### Authentication flow

When security is enabled:

1. Put the badge in BT-PAIRING mode — a 4-digit PIN is shown on the display.
2. Connect to the badge over BLE.
3. Send the PIN as a single write to characteristic `0xFEE1` before any `wang`
   packet:

| Byte  | Content                                                                |
|-------|------------------------------------------------------------------------|
| 0–3   | PIN digits as ASCII characters (e.g. `6512` → `0x36 0x35 0x31 0x32`)   |
| 4–15  | Zero padding                                                           |

4. If the PIN matches, all subsequent  transfers are accepted normally
   for the duration of the connection.
5. If the PIN does not match, the write is rejected and the transfer is blocked.
6. If a `wang` packet is sent before a correct PIN, it is rejected.

#### Backward compatibility

When security is **disabled** (default), the badge behaves identically to the
original OEM firmware — `wang` transfers are accepted directly without any
PIN. No changes are required in existing clients.

When security is **enabled**, clients that do not implement the PIN flow will
have their transfers rejected. The badge owner can also temporarily bypass the
PIN for a single session by pressing **KEY4** while in BT-PAIRING mode — this
allows legacy clients to connect without a PIN for that session only, reverting
to secure mode on the next disconnect.

#### Auth state lifecycle

- A new PIN is generated each time BT-PAIRING mode is entered.
- Auth state resets on every BLE disconnect — the PIN must be sent again on
  reconnect.
- The security setting (enabled/disabled) persists across reboots and is stored
  in flash.

#### Return codes

A PIN write to `0xFEE1` returns:

| Result                                         | ATT response                |
|------------------------------------------------|-----------------------------|
| PIN matched                                    | `SUCCESS` (`0x00`)          |
| PIN wrong                                      | `ATT_ERR_UNLIKELY` (`0x0E`) |
| Not a valid PIN attempt                        | `ATT_ERR_UNLIKELY` (`0x0E`) |
| `wang` sent before PIN when security enabled   | `ATT_ERR_UNLIKELY` (`0x0E`) |

#### Impact on third-party clients

Third-party libraries and tools (e.g. Python, Rust, Go) are unaffected as long
as the badge owner keeps security disabled (default). If security is enabled,
clients need to:

1. Detect the PIN prompt — the badge will reject the first `"wang"` write with
   an error if security is on.
2. Prompt the user to enter the PIN shown on the badge.
3. Send the PIN packet to `0xFEE1` as described above.
4. Retry the `wang` transfer.