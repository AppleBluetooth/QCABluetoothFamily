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

#include "AR3001BluetoothHostControllerUSBTransport.h"

#define super IOBluetoothHostControllerUSBTransport
OSDefineMetaClassAndStructors(AR3001BluetoothHostControllerUSBTransport, super)

bool AR3001BluetoothHostControllerUSBTransport::init(OSDictionary * dictionary)
{
	CreateOSLogObject();
    if ( !super::init() )
    {
        os_log(mInternalOSLogObject, "**** [AR3001BluetoothHostControllerUSBTransport][init] -- super::init() failed ****\n");
        return false;
    }
    return true;
}

bool AR3001BluetoothHostControllerUSBTransport::start(IOService * provider)
{
    OpenFirmwareManager * firmware;
    OSData * fwData;
    IOReturn err;
    
    if ( !super::start(provider) )
    {
        os_log(mInternalOSLogObject, "**** [AR3001BluetoothHostControllerUSBTransport][start] -- super::start() failed -- 0x%04x ****\n", ConvertAddressToUInt32(this));
        return false;
    }

	mControllerVendorType = 2;
    setProperty("ActiveBluetoothControllerVendor", "AR3001");

    firmware = OpenFirmwareManager::withName("ath3k-1.fw", fwCandidates, fwCount);
    if ( !firmware )
    {
        os_log(mInternalOSLogObject, "**** [AR3001BluetoothHostControllerUSBTransport][start] -- Failed to create an OpenFirmwareManager instance with file name - no memory -- 0x%04x ****\n", ConvertAddressToUInt32(this));
        return false;
    }
    
    fwData = firmware->getFirmwareUncompressed("ath3k-1.fw");
    if ( !fwData )
    {
        os_log(mInternalOSLogObject, "**** [AR3001BluetoothHostControllerUSBTransport][start] -- Firmware file \"%s\" not found -- 0x%04x ****\n", "ath3k-1.fw", ConvertAddressToUInt32(this));
        OSSafeReleaseNULL(firmware);
        return false;
    }
    
    err = loadFirmware(fwData);
    
    if ( firmware )
        firmware->removeFirmwares();
    OSSafeReleaseNULL(firmware);
    
    if ( !err )
    {
        os_log(mInternalOSLogObject, "**** [AR3001BluetoothHostControllerUSBTransport][start] -- Firmware failed to load: 0x%04X -- 0x%04x ****\n", err, ConvertAddressToUInt32(this));
        return false;
    }
    os_log(mInternalOSLogObject, "**** [AR3001BluetoothHostControllerUSBTransport][start] -- Firmware loaded successfully! -- 0x%04x ****\n", ConvertAddressToUInt32(this));
    return true;
}

IOReturn AR3001BluetoothHostControllerUSBTransport::loadFirmware(OSData * firmware)
{
    IOReturn err;
    IOBufferMemoryDescriptor * buffer;
    StandardUSB::DeviceRequest request;
    UInt32 size;
    UInt32 bytesTransferred;
    UInt32 sent = 0;
    UInt32 count = firmware->getLength();

    request.bmRequestType       = USBmakebmRequestType(kDeviceRequestDirectionOut, kDeviceRequestTypeVendor, kDeviceRequestRecipientDevice);
    request.bRequest            = kAth3KDeviceRequestDownload;
    request.wValue              = 0;
    request.wIndex              = 0;
    request.wLength             = kFirmwareHeaderSize;
    
    err = mBluetoothUSBHostDevice->deviceRequest(this, request, (void *) firmware->getBytesNoCopy(), bytesTransferred);
    if ( err )
    {
        os_log(mInternalOSLogObject, "**** [AR3001BluetoothHostControllerUSBTransport][loadFirmware] -- deviceRequest() failed: 0x%04X -- 0x%04x ****\n", err, ConvertAddressToUInt32(this));
        return false;
    }
    sent += kFirmwareHeaderSize;
    count -= kFirmwareHeaderSize;

    while (count)
    {
        // Workaround the compatibility issue with xHCI controller
        IOSleepWithLeeway(50, 100);

        size = min(count, kBulkDataSize);
        buffer = IOBufferMemoryDescriptor::withBytes((UInt8 *) firmware->getBytesNoCopy() + sent, size, kIODirectionOut);
        if ( !buffer )
        {
            os_log(mInternalOSLogObject, "**** [AR3001BluetoothHostControllerUSBTransport][loadFirmware] -- Failed to create an IOBufferMemoryDescriptor instance - no memory -- 0x%04x ****\n", ConvertAddressToUInt32(this));
            return false;
        }
        
        buffer->prepare();
        err = mBulkOutPipe->io(buffer, size, bytesTransferred);
        buffer->complete();
        OSSafeReleaseNULL(buffer);
        
        if (err || (bytesTransferred != size))
        {
            os_log(mInternalOSLogObject, "**** [AR3001BluetoothHostControllerUSBTransport][loadFirmware] -- Firmware loading failed: err = %d, bytesTransferred = %d, size = %d, count = %d ****\n", err, bytesTransferred, size, count);
            return false;
        }
        sent  += size;
        count -= size;
    }
    
    return true;
}

OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 0)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 1)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 2)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 3)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 4)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 5)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 6)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 7)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 8)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 9)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 10)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 11)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 12)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 13)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 14)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 15)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 16)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 17)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 18)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 19)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 20)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 21)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 22)
OSMetaClassDefineReservedUnused(AR3001BluetoothHostControllerUSBTransport, 23)
