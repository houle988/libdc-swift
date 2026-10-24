# Release Notes

## Unreleased — 2026-10-21

libdivecomputer updated to upstream `9e6c3c8` (2026-09-11). Maximum and average depth now come from the dive computer's own record when it provides them, instead of being calculated from the downloaded profile; the profile values are still used when a computer does not report them.

### New devices supported
- **Cressi Raffaello** (model 6) — new libdivecomputer descriptor and BLE filter entry, plus a `ComputerModel` entry in `DeviceConfiguration.supportedModels`

### Dive data improvements
- **Computer-reported maximum depth** (`DC_FIELD_MAXDEPTH`) — `DiveData.maxDepth` uses the depth the computer recorded, which catches peaks that fall between logged samples. Every supported family except Shearwater, which keeps the deepest profile sample: its non-PNF firmware records the maximum in whole metres/feet, coarser than its 0.1 m samples, and the wrapper cannot tell PNF from non-PNF
- **Computer-reported average depth** (`DC_FIELD_AVGDEPTH`) — `DiveData.avgDepth` uses the computer's average when it records one: Mares (iconHD, Genius, Horizon layouts), Cressi Goa family (scuba, nitrox and gauge modes), Heinrichs Weikamp OSTC (log version 0x21 and later), Divesoft (header V2), Deep Six (layout v1), McLean Extreme, Seac Screen, Oceanic i330R / i330R C / DSX
- Both fall back to the profile value (deepest sample, time-weighted average) when the computer reports nothing, or a non-finite or non-positive value
- **Oceanic DSX** (upstream) — minimum and maximum temperature, Bühlmann GF deco model, time-to-surface on deco samples, ppO₂ samples (`DC_SENSOR_NONE`) and CNS samples
- **Oceanic i770R** (upstream) — dive time, minimum temperature and salinity (fresh/salt)
- **Mares average depth** field (upstream) for the iconHD, Genius and Horizon layouts

### CCR setpoint per sample
- **`DiveProfilePoint.setpoint` is now the setpoint in effect at each sample** (new `SampleData.currentSetpoint`), instead of the last setpoint seen anywhere earlier in the dive. `DiveData.setpoint`, the dive's last reported setpoint, is unchanged
- **Per-sample backends** (`GenericParser.reportsSetpointEverySample(_:)`): Shearwater, DiveSystem iDive, McLean Extreme and TecDiving report `DC_SAMPLE_SETPOINT` in every sample while a setpoint is in effect and omit it otherwise (e.g. Shearwater on open circuit), so the setpoint is cleared after each sample: an open-circuit bailout sample has no setpoint
- **Change-only backends** (OSTC, Suunto EON Steel, Divesoft Freedom, Suunto D9, Deep Six) keep holding the setpoint until the next report. When the backend flags its diluents (OSTC, EON Steel, Divesoft), a `DC_SAMPLE_GASMIX` switch to a non-diluent, non-oxygen mix clears it, a setpoint reported while on open circuit is recorded but not applied, and a switch back to a diluent restores the last reported setpoint
- Divesoft is treated as change-only: it reports the setpoint only in `POINT_1` records and in setpoint-change events
- Families with no diluent flags (Suunto D9, Deep Six, Liquivision) cannot signal a bailout, so their setpoint stays held
- No API change: `DiveProfilePoint.setpoint` and `DiveData.setpoint` keep their types and names
- Verified: 20 open-circuit dives (Shearwater Teric and Perdix 2, Suunto EON Steel, Aqualung i770R, Scubapro G2) parse byte-identically to the previous revision; 3 Shearwater Petrel 3 CCR dives differ only in the per-sample setpoint (none on open-circuit bailout samples)

### Bug fixes
- **Fixed Cressi model identification** — Cressi computers advertise BLE names of the form `<hex model>_<hex>` (e.g. `6_1A2B`); the C bridge resolved every Cressi Goa-family computer to the first descriptor, Cressi Cartesio. The new `resolveCressiBLEName` mirrors libdivecomputer's `dc_match_cressi` exactly (case-insensitive, no leading zeros, ASCII hex only, listed models only)
- **Fixed repeated full downloads on the Cressi Raffaello** — its fingerprint was saved and looked up under different device-name keys; both keys now match

### Changes
- **Vendored libdivecomputer** `9b1cd20` (2026-08-31) → `9e6c3c8` (2026-09-11): `descriptor.c`, `mares_iconhd_parser.c` and `oceanic_atom2_parser.c`, byte-identical to upstream, no local C patches; `version.h` unchanged (0.10.0)
- **Oceanic DSX salinity is no longer reported** (upstream: the previous decoding was incorrect)
- `UPSTREAM.md` records the new snapshot, the depth and Cressi additions, and the re-checked `SZ_DECOMPRESS_MAX` decision

### Notes
- Depths are still in metres, converted by libdivecomputer from each computer's native unit
- Verified on an Aqualung i300C (78 dives): every stored maximum equals the value in the computer's raw dive record — 0.04–0.82 m deeper than the deepest logged sample on 63 dives, identical on the rest; average depth unchanged (the i300C records none)
- Verified on a Shearwater Perdix 3 re-download: maximum equals the deepest sample and average the profile average, unchanged apart from 2-decimal rounding in previously stored values
- No sanity bound is applied to the computer's values: a corrupt all-ones field is not filtered (rare — every supported family reads it as at least 638 m, except older 12-bit Oceanic models at 78 m)
- Families that report no average depth keep the profile average: Shearwater, Suunto EON Steel family, Scubapro (Uwatec Smart), Deepblu, Halcyon, Oceans S1, DiveSystem, and most Oceanic/Aqualung models
- For apps using this library: re-downloading existing dives replaces their stored max and average depth with the computer's values

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
