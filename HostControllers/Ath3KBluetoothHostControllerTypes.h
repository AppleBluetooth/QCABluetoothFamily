/*
 *  Released under "The GNU General Public License (GPL-2.0)"
 *
 *  Copyright (c) 2021 cjiang. All rights reserved.
 *  Copyright (C) 2015 Intel Corporation.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#pragma once

#include <IOKit/bluetooth/Bluetooth.h>

enum Ath3KDeviceRequests
{
    kAth3KDeviceRequestDownload      = 0x01,
    kAth3KDeviceRequestGetState      = 0x05,
    kAth3KDeviceRequestSetNormalMode = 0x07,
    kAth3KDeviceRequestGetVersion    = 0x09,
    kAth3KDeviceRequestSwitchPID     = 0x09
};

enum Ath3KFirmwareStates
{
    kAth3KFirmwareStateSysConfigUpdate = 0x40,
    kAth3KFirmwareStatePatchUpdate     = 0x80
};

#define kAth3KModeMask       0x3F
#define kAth3kModeNormalMode 0x0E

enum Ath3KRefClockFrequencies
{
    kAth3KRefClockFrequency26M  = 0x00,
    kAth3KRefClockFrequency40M  = 0x01,
    kAth3KRefClockFrequency19P2 = 0x02
};

#define kAth3KFirmwareNameLength 0xFF
#define kBulkDataSize            4096
#define kFirmwareHeaderSize      20

struct BluetoothAth3KVersionInfo
{
    SInt32  romVersion;
    SInt32  buildVersion;
    SInt32  ramVersion;
    UInt8   refClock;
    UInt8   reserved[7];
} __attribute__((packed));

struct BluetoothAth3KVendorCommand
{
    UInt8   opCode;
    SInt16  index;
    UInt8   dataSize;
    UInt8   data[251];
} __attribute__((packed));

#define kAth3KVendorCommandOpCodeWriteTag                       0x01
#define kAth3KVendorCommandIndexDeviceAddress                   0x01

enum Ath3KBluetoothHCICommands
{
    kAth3KBluetoothHCICommandSendVendorCommand                  = 0x000B,
    kAth3KBluetoothHCICommandSleep                              = 0x0004
};
