# Swift wrapper upstream tracking

This repository is a fork of [deepsealabs/libdc-swift](https://github.com/deepsealabs/libdc-swift).
The fork diverges significantly from upstream; **never do a wholesale merge** — cherry-pick only.

## Last upstream review

| Field | Value |
|---|---|
| Upstream commit | `20430bb` |
| Upstream date | 2026-07-14 |
| Upstream tag | — |
| Review date | 2026-07-23 |

## Changes cherry-picked from upstream

| Commit area | What was taken | Reason |
|---|---|---|
| `BLEManager.swift` | Vendor-service preference over Nordic UART in `didDiscoverServices` | Cressi advertises both Nordic UART (…CA9E) and its own service (…10B8); libdivecomputer requires the vendor service |
| `BLEManager.swift` | Populate `characteristicsByUUID` dictionary in `didDiscoverCharacteristics` | Required for `DC_IOCTL_BLE_CHARACTERISTIC_READ` lookups |
| `BLEManager.swift` | `readCharacteristic(byUUID:timeout:)` method | Satisfies `DC_IOCTL_BLE_CHARACTERISTIC_READ` calls from BLEBridge.m |
| `BLEManager.swift` | Route ioctl reads in `didUpdateValueFor` to `ioctlReadValue` slot | Prevents Cressi version-read packets from contaminating the SLIP receive queue |
| `BLEManager.swift` | `.withResponse` write confirmation via `writeConfirmSemaphore` | Ensures write errors surface synchronously instead of being silently dropped |
| `CoreBluetoothManagerProtocol.h` | `readCharacteristicByUUID:timeout:` protocol method | Expose `readCharacteristic` to ObjC callers in BLEBridge.m |
| `BLEBridge.m` | `DC_IOCTL_BLE_CHARACTERISTIC_READ` handler in `ble_ioctl` | Enables Cressi Goa / Cartesio / Leonardo BLE device open |

## Changes NOT taken from upstream (our version is better)

### `DiveLogRetriever.swift`
Upstream has three bugs that are already fixed in our fork:
- **Fingerprint allocator**: upstream uses `UnsafeMutablePointer.allocate()` (Swift allocator), which causes `free_device_data()` to crash with "pointer being freed was not allocated" because the C side calls `free()`. Our version uses `malloc()`.
- **Timer on retrieval queue**: upstream uses `Timer.scheduledTimer`, which never fires on a GCD thread without a RunLoop. Our version uses `DispatchSourceTimer`.
- **Sentinel fingerprint**: upstream returns a hardcoded sentinel on failure. Our version returns `nil`.
- **Clock sync**: upstream has no `syncClock` feature. Our version does.

### `GenericParser.swift`
Our version includes features upstream is missing entirely:
- Per-point tank pressures
- `DC_SAMPLE_LOCATION` with entry/exit GPS coordinates
- `DC_GASMIX_UNKNOWN` handling
- Gas-change event synthesis
- Salinity density precision fix
- `SAMPLE_EVENT_PO2` handling

### `Models/DeviceConfiguration.swift`
Our version includes devices upstream does not know about:
- Shearwater Perdix 3 (modelID 14)
- Seac and Halcyon dive computers
- Full Cressi model list
- Oceanic/Aqualung BLE name resolution (`resolveOceanicBLEName`, `bleNamePrefixes`, `resolveByBLENamePrefix`)

### `CoreBluetoothManagerProtocol.h`
- Upstream dropped `getDeviceName`. **Do not remove it** — it is required for `DC_IOCTL_BLE_GET_NAME` on Oceanic/Aqualung/Sherwood models (i300C, i550C, i770R, etc.).
- Upstream renamed `setTimeout:` to `setReadTimeout:`. **Do not rename it** — our ObjC/Swift bridging uses the existing selector throughout.

### `BLEBridge.m` — `DC_IOCTL_BLE_GET_NAME`
Upstream removed this handler. **Do not remove it** — Oceanic/Aqualung devices embed the peripheral's advertised name in the READMEMORY handshake, and without it the device answers with NAK (0xA5).

### `Logger.swift`
Upstream and our version are functionally equivalent (`onLog`/`LogEvent` sink). No change needed.

## How to review upstream for new changes

1. Check the upstream repo for commits since the last review commit above.
2. For each changed file, diff against our version and categorise: regression, already fixed, or genuinely new.
3. Cherry-pick only files / hunks that represent new capability not present in our fork.
4. **Never** cherry-pick changes that would revert any item in the "NOT taken" list above.
5. Update the "Last upstream review" table with the new commit SHA and date.
6. Build the project to confirm no regressions.

## Our intentional additions not present in upstream

| Area | Feature |
|---|---|
| `DiveLogRetriever.swift` | Clock synchronisation (`syncClock`) |
| `GenericParser.swift` | GPS coordinates, tank pressures, gas-change synthesis, salinity precision |
| `DeviceConfiguration.swift` | Shearwater Perdix 3, Seac, Halcyon, Oceanic BLE name resolution |
| `BLEManager.swift` | Auto-reconnect on unexpected disconnect |
| `BLEManager.swift` | `DispatchSourceTimer` for retrieval timeout (vs broken `Timer`) |
| `BLEBridge.m` | `DC_IOCTL_BLE_GET_NAME` for Oceanic/Aqualung handshake |
