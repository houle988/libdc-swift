# Swift wrapper upstream tracking

This repository is a fork of [deepsealabs/libdc-swift](https://github.com/deepsealabs/libdc-swift).
The fork diverges significantly from upstream; **never do a wholesale merge** — cherry-pick only.

## Last upstream review

| Field | Value |
|---|---|
| Upstream commit | `d7b503b` |
| Upstream date | 2026-07-29 |
| Upstream tag | — |
| Review date | 2026-07-29 |

## Changes cherry-picked from upstream

| Commit area | What was taken | Reason |
|---|---|---|
| `BLEManager.swift` | Vendor-service preference over Nordic UART in `didDiscoverServices` | Cressi advertises both Nordic UART (…CA9E) and its own service (…10B8); libdivecomputer requires the vendor service |
| `BLEManager.swift` | Populate `characteristicsByUUID` dictionary in `didDiscoverCharacteristics` | Required for `DC_IOCTL_BLE_CHARACTERISTIC_READ` lookups |
| `BLEManager.swift` | `readCharacteristic(byUUID:timeout:)` method | Satisfies `DC_IOCTL_BLE_CHARACTERISTIC_READ` calls from BLEBridge.m |
| `BLEManager.swift` | Route ioctl reads in `didUpdateValueFor` to `ioctlReadValue` slot | Prevents Cressi version-read packets from contaminating the SLIP receive queue |
| `BLEManager.swift` | `.withResponse` write confirmation via `writeConfirmSemaphore` | Ensures write errors surface synchronously instead of being silently dropped |
| `BLEManager.swift` | `write()` returns `Int` (0/1/2) instead of `Bool` | Lets `ble_write` distinguish transient timeout (1→`DC_STATUS_TIMEOUT`, Oceanic may retry) from hard failure (2→`DC_STATUS_IO`) |
| `CoreBluetoothManagerProtocol.h` | `readCharacteristicByUUID:timeout:` protocol method | Expose `readCharacteristic` to ObjC callers in BLEBridge.m |
| `CoreBluetoothManagerProtocol.h` | `writeData:` return type `BOOL`→`NSInteger` | Coordinates with the `write()` Int change |
| `BLEBridge.m` | `DC_IOCTL_BLE_CHARACTERISTIC_READ` handler in `ble_ioctl` | Enables Cressi Goa / Cartesio / Leonardo BLE device open |
| `BLEBridge.m` | `ble_write` switch on 0/1/2 from `writeData:` | Maps transient timeout to `DC_STATUS_TIMEOUT`, hard failure to `DC_STATUS_IO` |
| `BLEBridge.m` | `ble_read` returns `DC_STATUS_TIMEOUT` when peripheral still ready | Prevents libdivecomputer treating a BLE connection-interval extension as a fatal IO error; Oceanic/Mares (3 attempts, MAXRETRIES=2) and DiveSystem (10 attempts, MAXRETRIES=9) retry on TIMEOUT. Trade-off: a dead-but-still-connected link takes N×timeout longer to fail vs. immediate IO abort. Shearwater/Cressi/Suunto abort on TIMEOUT same as IO so no extra hang risk for those families. |
| `DeviceConfiguration.swift` | `connectionLock` NSLock around `openBLEDevice` | Serialises concurrent callers so at most one connection attempt races on the shared libdivecomputer state |
| `DiveLogRetriever.swift` | `useFingerprint: Bool = true` param on `retrieveDiveLogs` | Allows callers to force a full history re-download, bypassing the fingerprint-based early-exit |

## Changes NOT taken from upstream (our version is better)

### `DiveLogRetriever.swift`
Upstream has three bugs that are already fixed in our fork:
- **Fingerprint allocator**: upstream uses `UnsafeMutablePointer.allocate()` (Swift allocator), which causes `free_device_data()` to crash with "pointer being freed was not allocated" because the C side calls `free()`. Our version uses `malloc()`.
- **Timer on retrieval queue**: upstream uses `Timer.scheduledTimer`, which never fires on a GCD thread without a RunLoop. Our version uses `DispatchSourceTimer`.
- **Sentinel fingerprint**: upstream returns a hardcoded sentinel on failure. Our version returns `nil`.
- **Clock sync**: upstream has no `syncClock` feature. Our version does.

### `GenericParser.swift` + `Models/SampleData.swift`
Our version includes features and fixes upstream is missing entirely:
- Per-point tank pressures
- `DC_SAMPLE_LOCATION` with entry/exit GPS coordinates
- `DC_GASMIX_UNKNOWN` handling
- Gas-change event synthesis
- Salinity density precision fix
- `SAMPLE_EVENT_PO2` handling
- **Temperature sentinel** (commit `54aa0ee`): `SampleData.tempSurface` defaults to `Double.infinity` instead of `0`. **Do not revert to `0`** — 0°C is a valid surface temperature for ice dives and must not be treated as "no data". `GenericParser` guards all three optional `DiveData` temperature fields (`surfaceTemperature`, `minTemperature`, `maxTemperature`) with `.isFinite ? value : nil` before populating. Note: `DiveData.temperature` (non-optional `Double`) receives `tempMinimum` directly without an isFinite guard — callers must handle `.isInfinite` for dives where no minimum temperature was recorded by the device.

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
Our version (commit `ca792b8`) already has `PacketEvent`, `PacketDirection`, `onPacket`, and `logPacket` — fully in sync with upstream's `8cd22e0`. Our version additionally adds `enableDebugMode()`, `disableDebugMode()`, and `resetDataCounters()` which upstream lacks.

### `BLEManager.swift` — auto-reconnect
Upstream removed auto-reconnect in `2b6da5d`. **Do not remove it** — it is an intentional addition in our fork (see "Our intentional additions" table below).

### `BLEManager.swift` — `willRestoreState` + `CBCentralManagerOptionRestoreIdentifierKey`
Upstream added iOS CoreBluetooth state restoration so the OS can relaunch the host app in the background to deliver `willRestoreState`. **Do not take this change** until the host app declares `bluetooth-central` in its `UIBackgroundModes` (Info.plist). Without that entitlement the restore identifier is a no-op — `willRestoreState` is never called and the option adds overhead for zero benefit. If `bluetooth-central` is ever added, a full integration audit is required: a restored peripheral lands in `bleManager.peripheral` with `openedDeviceDataPtr == nil` and `connectedDevice == nil`; the host app's connection flow must be verified to handle or ignore this pre-populated state without a stuck session or leak.

### `libdivecomputer/src/shearwater_common.c` — `SZ_DECOMPRESS_MAX`
Upstream (`7033030`) added a 20 MB sanity ceiling on LRE decompression buffer growth to our vendored C library copy. This fix is **not present in the real libdivecomputer upstream** — it is a local patch applied only to the `libdc-swift` vendored copy. We skip it to avoid drift from the real libdivecomputer source; revisit at the next `libdivecomputer` sync.

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
| `GenericParser.swift` / `SampleData.swift` | GPS coordinates, tank pressures, gas-change synthesis, salinity precision; `Double.infinity` temperature sentinel (ice-diving 0°C fix) |
| `DeviceConfiguration.swift` | Shearwater Perdix 3, Seac, Halcyon, Oceanic BLE name resolution |
| `BLEManager.swift` | Auto-reconnect on unexpected disconnect |
| `BLEManager.swift` | `DispatchSourceTimer` for retrieval timeout (vs broken `Timer`) |
| `BLEBridge.m` | `DC_IOCTL_BLE_GET_NAME` for Oceanic/Aqualung handshake |
