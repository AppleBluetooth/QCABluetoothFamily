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

#include "AR3002BluetoothHostControllerUSBTransport.h"

#define super IOBluetoothHostControllerUSBTransport
OSDefineMetaClassAndStructors(AR3002BluetoothHostControllerUSBTransport, super)

bool AR3002BluetoothHostControllerUSBTransport::init(OSDictionary * dictionary)
{
	CreateOSLogObject();
    if ( !super::init() )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][init] -- super::init() failed ****\n");
        return false;
    }
    mFirmware = NULL;
    return true;
}

void AR3002BluetoothHostControllerUSBTransport::free()
{
    mFirmware = NULL;
    super::free();
}

IOService * AR3002BluetoothHostControllerUSBTransport::probe( IOService * provider, SInt32 * score )
{
    return super::probe(provider, score);
}

bool AR3002BluetoothHostControllerUSBTransport::start(IOService * provider)
{
    IOReturn err;
    
    if ( !super::start(provider) )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][start] -- super::start() failed -- 0x%04x ****\n", ConvertAddressToUInt32(this));
        return false;
    }
	mControllerVendorType = 8;
    setProperty("ActiveBluetoothControllerVendor", "Atheros - AR3002");
    
    if ( mBluetoothUSBHostDevice->getDeviceDescriptor()->bcdDevice > 0x0001 )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][start] -- Firmware is already loaded! -- 0x%04x ****\n", ConvertAddressToUInt32(this));
        return true;
    }
    
    err = loadPatch();
    if ( err )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][start] -- Failed to load patch file: 0x%04x -- 0x%04x ****\n", err, ConvertAddressToUInt32(this));
        return false;
    }
    err = loadSysConfig();
    if ( err )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][start] -- Failed to load system configuration file: 0x%04x -- 0x%04x ****\n", err, ConvertAddressToUInt32(this));
        return false;
    }
    err = setNormalMode();
    if ( err )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][start] -- Failed to set normal mode: 0x%04x -- 0x%04x ****\n", err, ConvertAddressToUInt32(this));
        return false;
    }
    switchPID();
    return true;
}

void AR3002BluetoothHostControllerUSBTransport::stop(IOService * provider)
{
    super::stop(provider);
    OSSafeReleaseNULL(mFirmware);
}

IOReturn AR3002BluetoothHostControllerUSBTransport::loadFirmware(OSData * firmware)
{
    IOReturn err;
    IOBufferMemoryDescriptor * buffer;
    StandardUSB::DeviceRequest request;
    UInt32 size;
    UInt32 bytesTransferred;
    UInt32 sent = 0;
    UInt32 count = firmware->getLength();

    size = min(count, kFirmwareHeaderSize);
    request.bmRequestType       = USBmakebmRequestType(kDeviceRequestDirectionOut, kDeviceRequestTypeVendor, kDeviceRequestRecipientDevice);
    request.bRequest            = kAth3KDeviceRequestDownload;
    request.wValue              = 0;
    request.wIndex              = 0;
    request.wLength             = size;
    
    err = mBluetoothUSBHostDevice->deviceRequest(this, request, (void *) firmware->getBytesNoCopy(), bytesTransferred);
    if (err)
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][loadFirmware] -- deviceRequest() failed: 0x%04X -- 0x%04x ****\n", err, ConvertAddressToUInt32(this));
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
            os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][loadFirmware] -- Failed to create an IOBufferMemoryDescriptor instance - no memory -- 0x%04x ****\n", ConvertAddressToUInt32(this));
            return false;
        }
        
        buffer->prepare();
        err = mBulkOutPipe->io(buffer, size, bytesTransferred);
        buffer->complete();
        OSSafeReleaseNULL(buffer);
        
        if (err || (bytesTransferred != size))
        {
            os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][loadFirmware] -- Firmware loading failed: err = %d, bytesTransferred = %d, size = %d, count = %d ****\n", err, bytesTransferred, size, count);
            return false;
        }
        sent  += size;
        count -= size;
    }
    
    return true;
}

IOReturn AR3002BluetoothHostControllerUSBTransport::loadPatch()
{
    IOReturn result;
    UInt8 fwState;
    char filename[kAth3KFirmwareNameLength];
    OSData * fwData;
    BluetoothAth3KVersionInfo fwVersion;
    UInt32 ptROMVersion, ptBuildVersion;
    
    result = getState(&fwState);
    if ( result )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][loadPatch] -- Failed to obtain the firmware state: 0x%04x -- 0x%04x ****\n", result, ConvertAddressToUInt32(this));
        return result;
    }

    if ( fwState & kAth3KFirmwareStatePatchUpdate )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][loadPatch] -- Patch is already loaded! -- 0x%04x ****\n", ConvertAddressToUInt32(this));
        return kIOReturnSuccess;
    }

    result = getVersion(&fwVersion);
    if ( result )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][loadPatch] -- Failed to obtain the firmware version: 0x%04x -- 0x%04x ****\n", result, ConvertAddressToUInt32(this));
        return result;
    }

    snprintf(filename, kAth3KFirmwareNameLength, "AthrBT_0x%08x.dfu", (UInt32) (fwVersion.romVersion));

    mFirmware = OpenFirmwareManager::withName(filename, fwCandidates, fwCount);
    if ( !mFirmware )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][loadPatch] -- Failed to create an OpenFirmwareManager instance with file name - no memory -- 0x%04x ****\n", ConvertAddressToUInt32(this));
        return kIOReturnNoMemory;
    }
    if ( !mFirmware->getFirmwareUncompressed(filename) )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][loadPatch] -- Patch file \"%s\" not found -- 0x%04x ****\n", filename, ConvertAddressToUInt32(this));
        OSSafeReleaseNULL(mFirmware);
        return kIOReturnUnsupported;
    }

    fwData = mFirmware->getFirmwareUncompressed(filename);
    ptROMVersion = *(UInt32 *) ((UInt8 *) fwData->getBytesNoCopy() + fwData->getLength() - 8);
    ptBuildVersion = *(UInt32 *) ((UInt8 *) fwData->getBytesNoCopy() + fwData->getLength() - 4);

    if (ptROMVersion != (UInt32) (fwVersion.romVersion) || ptBuildVersion <= (UInt32) (fwVersion.buildVersion))
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][loadPatch] -- Patch file version did not match with firmware version -- 0x%04x ****\n", ConvertAddressToUInt32(this));
        mFirmware->removeFirmwares();
        OSSafeReleaseNULL(mFirmware);
        return kIOReturnInvalid;
    }

    result = loadFirmware(mFirmware->getFirmwareUncompressed(filename));
    mFirmware->removeFirmwares();
    OSSafeReleaseNULL(mFirmware);
    
    return result;
}

IOReturn AR3002BluetoothHostControllerUSBTransport::loadSysConfig()
{
    IOReturn result;
    UInt8 fwState;
    char filename[kAth3KFirmwareNameLength];
    BluetoothAth3KVersionInfo fwVersion;
    int clkValue;

    result = getState(&fwState);
    if ( result )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][loadSysConfig] -- Failed to obtain the firmware state: 0x%04x -- 0x%04x ****\n", result, ConvertAddressToUInt32(this));
        return kIOReturnBusy;
    }

    result = getVersion(&fwVersion);
    if ( result )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][loadSysConfig] -- Failed to obtain the firmware version: 0x%04x -- 0x%04x ****\n", result, ConvertAddressToUInt32(this));
        return result;
    }

    switch (fwVersion.refClock)
    {
        case kAth3KRefClockFrequency26M:
            clkValue = 26;
            break;
        case kAth3KRefClockFrequency40M:
            clkValue = 40;
            break;
        case kAth3KRefClockFrequency19P2:
            clkValue = 19;
            break;
        default:
            clkValue = 0;
            break;
    }

    snprintf(filename, kAth3KFirmwareNameLength, "ramps_0x%08x_%d%s", (UInt32) (fwVersion.romVersion), clkValue, ".dfu");

    mFirmware = OpenFirmwareManager::withName(filename, fwCandidates, fwCount);
    if ( !mFirmware )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][loadSysConfig] -- Failed to create an OpenFirmwareManager instance with file name - no memory -- 0x%04x ****\n", ConvertAddressToUInt32(this));
        return kIOReturnNoMemory;
    }
    if ( !mFirmware->getFirmwareUncompressed(filename) )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][loadSysConfig] -- System configuration file \"%s\" not found -- 0x%04x ****\n", filename, ConvertAddressToUInt32(this));
        OSSafeReleaseNULL(mFirmware);
        return kIOReturnUnsupported;
    }

    result = loadFirmware(mFirmware->getFirmwareUncompressed(filename));
    mFirmware->removeFirmwares();
    OSSafeReleaseNULL(mFirmware);

    return result;
}

IOReturn AR3002BluetoothHostControllerUSBTransport::getState(UInt8 * state)
{
    IOReturn err;
    StandardUSB::DeviceRequest request =
    {
        .bmRequestType      = USBmakebmRequestType(kDeviceRequestDirectionIn, kDeviceRequestTypeVendor, kDeviceRequestRecipientDevice),
        .bRequest           = kAth3KDeviceRequestGetState,
        .wValue             = 0,
        .wIndex             = 0,
        .wLength            = sizeof(UInt8)
    };
    
    err = mBluetoothUSBHostDevice->deviceRequest(this, request, state, (IOUSBHostCompletion *) NULL);
    if (err)
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][getState] -- deviceRequest() failed: 0x%04X -- 0x%04x ****\n", err, ConvertAddressToUInt32(this));
        return err;
    }
    return kIOReturnSuccess;
}

IOReturn AR3002BluetoothHostControllerUSBTransport::getVersion(BluetoothAth3KVersionInfo * version)
{
    IOReturn err;
    StandardUSB::DeviceRequest request =
    {
        .bmRequestType      = USBmakebmRequestType(kDeviceRequestDirectionIn, kDeviceRequestTypeVendor, kDeviceRequestRecipientDevice),
        .bRequest           = kAth3KDeviceRequestGetVersion,
        .wValue             = 0,
        .wIndex             = 0,
        .wLength            = sizeof(BluetoothAth3KVersionInfo)
    };
    
    err = mBluetoothUSBHostDevice->deviceRequest(this, request, version, (IOUSBHostCompletion *) NULL);
    if (err)
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][getVersion] -- deviceRequest() failed: 0x%04X -- 0x%04x ****\n", err, ConvertAddressToUInt32(this));
        return err;
    }
    return kIOReturnSuccess;
}

void AR3002BluetoothHostControllerUSBTransport::switchPID()
{
    StandardUSB::DeviceRequest request =
    {
        .bmRequestType      = USBmakebmRequestType(kDeviceRequestDirectionOut, kDeviceRequestTypeVendor, kDeviceRequestRecipientDevice),
        .bRequest           = kAth3KDeviceRequestSwitchPID,
        .wValue             = 0,
        .wIndex             = 0,
        .wLength            = 0
    };
    
    mBluetoothUSBHostDevice->deviceRequest(this, request, (void *) NULL, (IOUSBHostCompletion *) NULL);
}

IOReturn AR3002BluetoothHostControllerUSBTransport::setNormalMode()
{
    IOReturn result;
    UInt8 fwState;
    StandardUSB::DeviceRequest request;
    
    result = getState(&fwState);
    if ( result )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][setNormalMode] -- getState() failed: 0x%04X -- 0x%04x ****\n", result, ConvertAddressToUInt32(this));
        return result;
    }

    if ( (fwState & kAth3KModeMask) == kAth3kModeNormalMode )
    {
        os_log(mInternalOSLogObject, "**** [AR3002BluetoothHostControllerUSBTransport][setNormalMode] -- Firmware is already in normal mode! -- 0x%04x ****\n", ConvertAddressToUInt32(this));
        return kIOReturnSuccess;
    }
    
    request.bmRequestType   = USBmakebmRequestType(kDeviceRequestDirectionOut, kDeviceRequestTypeVendor, kDeviceRequestRecipientDevice);
    request.bRequest        = kAth3KDeviceRequestSetNormalMode;
    request.wValue          = 0;
    request.wIndex          = 0;
    request.wLength         = 0;

    return mBluetoothUSBHostDevice->deviceRequest(this, request, (void *) NULL, (IOUSBHostCompletion *) NULL);
}

OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 0)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 1)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 2)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 3)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 4)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 5)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 6)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 7)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 8)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 9)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 10)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 11)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 12)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 13)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 14)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 15)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 16)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 17)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 18)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 19)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 20)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 21)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 22)
OSMetaClassDefineReservedUnused(AR3002BluetoothHostControllerUSBTransport, 23)

