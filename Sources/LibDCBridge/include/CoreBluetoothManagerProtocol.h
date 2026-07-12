#ifndef CoreBluetoothManagerProtocol_h
#define CoreBluetoothManagerProtocol_h

#ifdef __OBJC__
#import <Foundation/Foundation.h>

@protocol CoreBluetoothManagerProtocol <NSObject>
+ (id)shared;
- (BOOL)connectToDevice:(NSString *)address;
- (BOOL)getPeripheralReadyState;
- (BOOL)discoverServices;
- (BOOL)enableNotifications;
- (BOOL)writeData:(NSData *)data;
- (NSData *)readDataPartial:(int)requested;
- (void)setTimeout:(int)timeoutMs;
- (void)close;
/// Returns the connected peripheral's advertised Bluetooth name (e.g.
/// "FH020399"), or an empty string if no peripheral is connected.
/// Required by libdivecomputer's DC_IOCTL_BLE_GET_NAME for Oceanic /
/// Aqualung models (i300C, i550C, i770R, etc.) that embed digits of the
/// serial number in the advertised name and use it during the protocol
/// handshake.  Without this the READMEMORY command is answered with NAK
/// (0xA5) instead of an ACK + payload.
- (NSString *)getDeviceName;
/// Read a secondary BLE characteristic by UUID and return its value.
/// Used by BLEBridge.m to handle DC_IOCTL_BLE_CHARACTERISTIC_READ for
/// devices like Cressi Goa that expose serial/model/firmware info on
/// non-data characteristics.  Blocks until a value is received or the
/// timeout (in seconds) expires; returns nil on failure.
- (NSData *)readCharacteristicByUUID:(NSString *)uuid timeout:(double)seconds;
@end

#else
// If we're compiling pure C (without Objective-C), provide an empty protocol definition
typedef void * CoreBluetoothManagerProtocol;
#endif

#endif /* CoreBluetoothManagerProtocol_h */ 