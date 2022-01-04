/*
 *  Released under "The GNU General Public License (GPL-2.0)"
 *
 *  Copyright (c) 2021 cjiang. All rights reserved.
 *  Copyright (c) 2008-2009 Atheros Communications Inc.
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

#ifndef AR3002BluetoothHostControllerUSBTransport_h
#define AR3002BluetoothHostControllerUSBTransport_h

#include <IOKit/usb/USB.h>
#include <IOKit/bluetooth/transport/IOBluetoothHostControllerUSBTransport.h>
#include <Ath3KBluetoothHostControllerTypes.h>
#include <FirmwareList.h>

class AR3002BluetoothHostControllerUSBTransport : public IOBluetoothHostControllerUSBTransport
{
    OSDeclareDefaultStructors(AR3002BluetoothHostControllerUSBTransport)
    
public:
    virtual bool init( OSDictionary * dictionary = NULL ) APPLE_KEXT_OVERRIDE;
    virtual void free() APPLE_KEXT_OVERRIDE;
    virtual IOService * probe( IOService * provider, SInt32 * score ) APPLE_KEXT_OVERRIDE;
    virtual bool start( IOService * provider ) APPLE_KEXT_OVERRIDE;
    virtual void stop( IOService * provider ) APPLE_KEXT_OVERRIDE;
    
    virtual IOReturn loadFirmware( OSData * firmware );
    virtual IOReturn loadPatch();
    virtual IOReturn loadSysConfig();
    virtual IOReturn getState(UInt8 * state);
    virtual IOReturn getVersion(BluetoothAth3KVersionInfo * version);
    virtual void switchPID();
    virtual IOReturn setNormalMode();
    
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 0);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 1);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 2);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 3);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 4);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 5);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 6);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 7);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 8);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 9);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 10);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 11);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 12);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 13);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 14);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 15);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 16);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 17);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 18);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 19);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 20);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 21);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 22);
    OSMetaClassDeclareReservedUnused(AR3002BluetoothHostControllerUSBTransport, 23);
    
protected:
    OpenFirmwareManager * mFirmware;
};

#endif
