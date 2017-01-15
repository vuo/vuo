/**
 * @file
 * VuoSerialIO implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoSerial.h"
#include "VuoPool.hh"
#include "VuoTriggerSet.hh"

#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <termios.h>
#include <IOKit/serial/ioss.h>
#include <errno.h>

#include <map>
#include <string>

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoSerialIO",
					 "dependencies" : [
						 "VuoSerialDevice",
						 "VuoList_VuoSerialDevice"
					 ]
				 });
#endif
}


/**
 * Private data for a VuoSerial instance.
 */
typedef struct _VuoSerial_internal
{
	VuoText devicePath;		///< The target device this instance represents.
	dispatch_queue_t queue;
	VuoTriggerSet<VuoData> triggerSet;
	dispatch_source_t source;
	int fileHandle;
} *VuoSerial_internal;


static dispatch_queue_t VuoSerial_pendingDevicesQueue;	///< Serializes access to `VuoSerial_pendingDevices`.
static std::set<VuoSerial_internal> VuoSerial_pendingDevices;	///< Devices that have been requested but haven't yet been successfully opened.


/**
 * Initializes `VuoSerial_pendingDevicesQueue`.
 */
static void __attribute__((constructor)) VuoSerialIO_init()
{
	VuoSerial_pendingDevicesQueue = dispatch_queue_create("org.vuo.serial.pendingDevicesQueue", NULL);
}


/**
 * Reads and parses a single serial packet from the Dispatch Source's file handle.
 *
 * @threadQueue{VuoSerial_internal::queue}
 */
void VuoSerial_processPacket(VuoSerial_internal si)
{
	ssize_t bytesRead = 0;
	int fd = -1;
	int bytesAvailable = dispatch_source_get_data(si->source);
	if (bytesAvailable == 0)
		goto fail;

	fd = dispatch_source_get_handle(si->source);
	VuoData data;
	data.data = (char *)malloc(bytesAvailable);
	VuoRegister(data.data, free);
	bytesRead = read(fd, data.data, bytesAvailable);
	if (bytesRead < 1)
	{
		VuoRetain(data.data);
		VuoRelease(data.data);
		goto fail;
	}

	data.size = bytesRead;

#if 0
{
	char *hex = (char *)malloc(data.size * 4 + 2);
	bzero(hex, data.size * 4 + 2);
	for(int i = 0; i < data.size; ++i)
	{
		hex[i] = isprint(data.data[i]) ? data.data[i] : '_';
		sprintf(hex + data.size + 1 + i*3, "%02x ", (unsigned char)data.data[i]);
	}
	hex[data.size] = '\t';
	VUserLog("%4lld:\t%s", data.size, hex);
	free(hex);
}
#endif

	si->triggerSet.fire(data);
	return;

fail:
//	if (bytesRead == 0)
//		VLog("Error: End of file");
//	else
//		VLog("Error: %s", strerror(errno));

	dispatch_source_cancel(si->source);
	dispatch_release(si->source);
	si->source = NULL;

	dispatch_async(VuoSerial_pendingDevicesQueue, ^{
		VuoSerial_pendingDevices.insert(si);
	});
}


/**
 * Sets up a Dispatch Source for the specified file handle.
 */
static void VuoSerial_initSource(VuoSerial_internal si)
{
	si->source = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, si->fileHandle, 0, si->queue);
	dispatch_source_set_event_handler(si->source, ^{
		VuoSerial_processPacket(si);
	});
	dispatch_source_set_cancel_handler(si->source, ^{
		close(si->fileHandle);
	});
	dispatch_resume(si->source);
}

/**
 * Attempts to open the specified device.
 */
static int VuoSerial_openDevice(const char *devicePath)
{
	return open(devicePath,
		O_RDWR			// In case the composition author ever creates both a Send and Receive node for the same device.
		| O_NOCTTY		// Never change this process's controlling terminal.
		| O_NONBLOCK	// Avoid blocking while opening (if the device is exclusively locked by another process, that could take a long time, and VuoRunner might think the process has hung).
		| O_EXLOCK		// Prevent corruption if multiple processes try to read from the same serial device (each process only gets some of the data).
	);
}

/**
 * Attempts to open each device that has been requested but hasn't yet been successfully opened.
 *
 * @threadAny
 */
void VuoSerial_checkPendingDevices(void)
{
	dispatch_async(VuoSerial_pendingDevicesQueue, ^{
		for (std::set<VuoSerial_internal>::iterator i = VuoSerial_pendingDevices.begin(); i != VuoSerial_pendingDevices.end();)
		{
			(*i)->fileHandle = VuoSerial_openDevice((*i)->devicePath);
			if ((*i)->fileHandle < 0)
			{
				++i;
				continue;
			}

			VuoSerial_initSource(*i);
			VuoSerial_pendingDevices.erase(i++);
		}
	});
}


/// @{
VUOKEYEDPOOL(std::string, VuoSerial_internal);
static void VuoSerial_destroy(VuoSerial_internal si);
VuoSerial_internal VuoSerial_make(std::string devicePath)
{
//	VLog("%s", devicePath.c_str());

	VuoSerial_use();

	VuoSerial_internal si = new _VuoSerial_internal;
	VuoRegister(si, (DeallocateFunctionType)VuoSerial_destroy);

	si->devicePath = VuoText_make(devicePath.c_str());
	VuoRetain(si->devicePath);

	si->queue = dispatch_queue_create("org.vuo.serial", NULL);

	si->source = NULL;

	si->fileHandle = VuoSerial_openDevice(si->devicePath);
	if (si->fileHandle >= 0)
		VuoSerial_initSource(si);
	else
	{
		VUserLog("Warning: Serial device '%s' isn't currently available (%s).  I'll keep trying.", si->devicePath, strerror(errno));
		dispatch_async(VuoSerial_pendingDevicesQueue, ^{
			VuoSerial_pendingDevices.insert(si);
		});
	}

	return si;
}
static void VuoSerial_destroy(VuoSerial_internal si)
{
//	VLog("%s", si->devicePath);

	dispatch_sync(VuoSerial_pendingDevicesQueue, ^{
		VuoSerial_pendingDevices.erase(si);
	});

	if (si->source)
	{
		dispatch_sync(si->queue, ^{
			dispatch_source_cancel(si->source);
			dispatch_release(si->source);
		});
	}

	dispatch_release(si->queue);

	VuoSerial_internalPool->removeSharedInstance(si->devicePath);

	VuoRelease(si->devicePath);
	delete si;

	VuoSerial_disuse();
}
VUOKEYEDPOOL_DEFINE(std::string, VuoSerial_internal, VuoSerial_make);
/// @}
///


/**
 * Returns the reference-counted object for the specified serial device.
 */
VuoSerial VuoSerial_getShared(const VuoText devicePath)
{
	return (VuoSerial)VuoSerial_internalPool->getSharedInstance(devicePath);
}


/**
 * Adds a trigger callback, to be invoked whenever the specified serial device receives data.
 *
 * @threadAny
 */
void VuoSerial_addReceiveTrigger(VuoSerial device, VuoOutputTrigger(receivedData, VuoData))
{
	if (!device)
		return;

	VuoSerial_internal si = (VuoSerial_internal)device;
//	VLog("%s", si->devicePath);
	si->triggerSet.addTrigger(receivedData);
}


/**
 * Removes a trigger callback previously added by @ref VuoSerial_addReceiveTrigger.
 *
 * @threadAny
 */
void VuoSerial_removeReceiveTrigger(VuoSerial device, VuoOutputTrigger(receivedData, VuoData))
{
	if (!device)
		return;

	VuoSerial_internal si = (VuoSerial_internal)device;
//	VLog("%s", si->devicePath);
	si->triggerSet.removeTrigger(receivedData);
}

/**
 * Enqueues `data` for output on `device`.
 */
void VuoSerial_sendData(VuoSerial device, const VuoData data)
{
	if (!device || !data.size || !data.data)
		return;

	VuoSerial_internal si = (VuoSerial_internal)device;
	dispatch_async(si->queue, ^{
					   write(si->fileHandle, data.data, data.size);
				   });
}

/**
 * Changes system-wide settings for the specified serial device.
 */
void VuoSerial_configureDevice(VuoSerial device, VuoInteger baudRate, VuoInteger dataBits, VuoParity parity, VuoInteger stopBits)
{
	if (!device)
		return;

	VuoSerial_internal si = (VuoSerial_internal)device;
//	VLog("%s", si->devicePath);


	// Configure baud rate.
	if (ioctl(si->fileHandle, IOSSIOSPEED, &baudRate) == -1)
		VUserLog("Couldn't set %s to baud rate %lld: %s", si->devicePath, baudRate, strerror(errno));


	// Get the current options.
	struct termios options;
	if (tcgetattr(si->fileHandle, &options) == -1)
	{
		VUserLog("Couldn't get TTY attributes for %s: %s", si->devicePath, strerror(errno));
		return;
	}


	// Configure data bits.
	options.c_cflag &= ~CSIZE;
	if (dataBits == 5)
		options.c_cflag |= CS5;
	else if (dataBits == 6)
		options.c_cflag |= CS6;
	else if (dataBits == 7)
		options.c_cflag |= CS7;
	else if (dataBits == 8)
		options.c_cflag |= CS8;
	else
		VUserLog("Couldn't set %s dataBits to %lld.", si->devicePath, dataBits);


	// Configure parity.
	if (parity == VuoParity_None)
		options.c_cflag &= ~PARENB;	// Clear the Parity Enable bit.
	else if (parity == VuoParity_Even)
	{
		options.c_cflag |= PARENB;	// Set the Parity Enable bit.
		options.c_cflag &= ~PARODD;	// Clear the Parity Odd bit.
	}
	else if (parity == VuoParity_Odd)
		options.c_cflag |= PARENB | PARODD;	// Set the Parity Enable and Parity Odd bits.
	else
		VUserLog("Couldn't set %s parity to %d.", si->devicePath, parity);


	// Configure stop bits.
	if (stopBits == 1)
		options.c_cflag &= ~CSTOPB;
	else if (stopBits == 2)
		options.c_cflag |= CSTOPB;
	else
		VUserLog("Couldn't set %s stopBits to %lld.", si->devicePath, stopBits);


	// Apply configuration immediately.
	if (tcsetattr(si->fileHandle, TCSANOW, &options) == -1)
		VUserLog("Couldn't configure %s: %s", si->devicePath, strerror(errno));
}
