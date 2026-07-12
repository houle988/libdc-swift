# Release Notes

## v2.0 — 2026-07-10

### New devices supported
- **Shearwater Perdix 3** — uses a new BLE service UUID distinct from earlier Shearwater models
- **Seac Screen** (advertised as "Tablet") — full dive log download support
- **Halcyon Symbios HUD** and **Halcyon Symbios Handset** — both variants supported

### New features
- **Device clock sync** — after every successful download the dive computer's internal clock is automatically updated to your phone's current time and timezone. Keeps dive timestamps accurate without manual intervention. Works on all devices that support `dc_device_timesync`; silently skipped on those that don't.
- **Auto-reconnect** — if a paired dive computer disconnects unexpectedly (device goes to sleep, moved out of range), the app reconnects automatically without user action. Reconnect is suppressed during an active download to avoid data corruption.

### Bug fixes
- **Fixed crash after downloading dives** — memory allocated by Swift was being freed by C's `free()`, causing a heap corruption crash on disconnect. Affected all devices on Apple Silicon.
- **Fixed hung downloads on connection drop** — if the BLE link dropped mid-transfer the download would hang indefinitely instead of timing out and showing an error. Now times out cleanly and lets you retry.
- **Fixed Oceanic / Aqualung / Sherwood downloads** — devices including the i300C, i550C, i770R, i200C, Oceanic Geo 4.0, and Sherwood Wisdom 4 were broken after a library update removed a required BLE handshake step. All affected models are working again.
- **Fixed Cressi connection stability** — during Cressi device setup, internal version-read packets were incorrectly treated as dive data, causing spurious wake-ups in the receive loop. Connection is now more stable.
- **Fixed stale BLE state after reconnect** — if a reconnect attempt found no recognised service, the previous connection's BLE characteristics were left behind, causing "Unknown ATT error" on the next protocol write. State is now fully reset on every reconnect.
- **Fixed write confirmation timeout under high BLE retry load** — in rare cases involving multiple prior write timeouts, a confirmed write could be incorrectly reported as failed. Each write attempt now gets a fresh timeout window.

### Dive data improvements
- **Tank pressure per sample** — per-point tank pressure is now recorded for computers that report it, enabling pressure graphs over the dive timeline
- **GPS entry and exit coordinates** — dive computers with GPS (e.g. Shearwater Teric with GPS pod) now record both the entry and exit surface location
- **Gas-change event synthesis** — gas switches are now recorded as explicit events in the dive profile, even on computers that report gas changes through the sample stream rather than a dedicated event channel
- **Salinity precision** — water density is now stored at full precision (e.g. 1.020 g/cm³ for EN13319 pool water) rather than being collapsed to a generic salt/fresh constant
- **PO2 events** — oxygen partial pressure alerts from CCR (rebreather) computers are now recorded in the event log
