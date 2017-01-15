/**
 * @file
 * VuoIoReturn implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoIoReturn.h"

#include <stdio.h>
#include <string.h>

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoIoReturn"
				 });
#endif


/**
 * Returns a string containing a verbal description of the specified IOReturn code.
 *
 * The caller is responsible for freeing the returned string.
 */
char *VuoIoReturn_getText(IOReturn ret)
{
	if (err_get_code(ret) == err_get_code(kIOReturnSuccess))
		return strdup("OK");

	int sys = err_get_system(ret);
	char *sysText;
	if (sys == err_get_system(sys_iokit)) sysText = strdup("IOKit");
	else
		asprintf(&sysText, "unknown(0x%04x)", sys);

	int sub = err_get_sub(ret);
	char *subText;
	if      (sub == err_get_sub(sub_iokit_common))			subText = strdup("Common");
	else if (sub == err_get_sub(sub_iokit_usb))				subText = strdup("USB");
	else if (sub == err_get_sub(sub_iokit_firewire))		subText = strdup("Firewire");
	else if (sub == err_get_sub(sub_iokit_block_storage))	subText = strdup("Storage");
	else if (sub == err_get_sub(sub_iokit_graphics))		subText = strdup("Graphics");
	else if (sub == err_get_sub(sub_iokit_networking))		subText = strdup("Networking");
	else if (sub == err_get_sub(sub_iokit_bluetooth))		subText = strdup("Bluetooth");
	else if (sub == err_get_sub(sub_iokit_pmu))				subText = strdup("PMU");
	else if (sub == err_get_sub(sub_iokit_acpi))			subText = strdup("ACPI");
	else if (sub == err_get_sub(sub_iokit_smbus))			subText = strdup("SMBUS");
	else if (sub == err_get_sub(sub_iokit_ahci))			subText = strdup("AHCI");
	else if (sub == err_get_sub(sub_iokit_powermanagement))	subText = strdup("Power Management");
	else if (sub == err_get_sub(err_sub(14) /*sub_iokit_hidsystem*/))		subText = strdup("HID System");
	else if (sub == err_get_sub(sub_iokit_scsi))			subText = strdup("SCSI");
	else if (sub == err_get_sub(sub_iokit_thunderbolt))		subText = strdup("Thunderbolt");
	else if (sub == err_get_sub(err_sub(0x45) /*sub_iokit_audio_video*/))	subText = strdup("Audio/Video");
	else if (sub == err_get_sub(err_sub(0x147) /*sub_iokit_hsic*/))			subText = strdup("HSIC");
	else if (sub == err_get_sub(err_sub(0x174) /*sub_iokit_sdio*/))			subText = strdup("SDIO");
	else if (sub == err_get_sub(err_sub(0x208) /*sub_iokit_wlan*/))			subText = strdup("WLAN");
	else if (sub == err_get_sub(sub_iokit_vendor_specific))	subText = strdup("Vendor-specific");
	else if (sub == err_get_sub(sub_iokit_reserved))		subText = strdup("Reserved");
	else asprintf(&subText, "unknown(0x%04x)", sub);

	int err = err_get_code(ret);
	char *errText;
	if		(err == err_get_code(kIOReturnError))				errText = strdup("general error");
	else if (err == err_get_code(kIOReturnNoMemory))			errText = strdup("can't allocate memory");
	else if (err == err_get_code(kIOReturnNoResources))			errText = strdup("resource shortage");
	else if (err == err_get_code(kIOReturnIPCError))			errText = strdup("error during IPC");
	else if (err == err_get_code(kIOReturnNoDevice))			errText = strdup("no such device");
	else if (err == err_get_code(kIOReturnNotPrivileged))		errText = strdup("privilege violation");
	else if (err == err_get_code(kIOReturnBadArgument))	   		errText = strdup("invalid argument");
	else if (err == err_get_code(kIOReturnLockedRead))	   		errText = strdup("device read locked");
	else if (err == err_get_code(kIOReturnLockedWrite))	   		errText = strdup("device write locked");
	else if (err == err_get_code(kIOReturnExclusiveAccess))		errText = strdup("exclusive access and device already open");
	else if (err == err_get_code(kIOReturnBadMessageID))		errText = strdup("sent/received messages had different msg_id");
	else if (err == err_get_code(kIOReturnUnsupported))			errText = strdup("unsupported function");
	else if (err == err_get_code(kIOReturnVMError))				errText = strdup("misc. VM failure");
	else if (err == err_get_code(kIOReturnInternalError))		errText = strdup("internal error");
	else if (err == err_get_code(kIOReturnIOError))				errText = strdup("general I/O error");
	else if (err == err_get_code(kIOReturnCannotLock))			errText = strdup("can't acquire lock");
	else if (err == err_get_code(kIOReturnNotOpen))				errText = strdup("device not open");
	else if (err == err_get_code(kIOReturnNotReadable))			errText = strdup("read not supported");
	else if (err == err_get_code(kIOReturnNotWritable))			errText = strdup("write not supported");
	else if (err == err_get_code(kIOReturnNotAligned))			errText = strdup("alignment error");
	else if (err == err_get_code(kIOReturnBadMedia))			errText = strdup("media error");
	else if (err == err_get_code(kIOReturnStillOpen))			errText = strdup("device(s) still open");
	else if (err == err_get_code(kIOReturnRLDError))			errText = strdup("rld failure");
	else if (err == err_get_code(kIOReturnDMAError))			errText = strdup("DMA failure");
	else if (err == err_get_code(kIOReturnBusy))				errText = strdup("device busy");
	else if (err == err_get_code(kIOReturnTimeout))				errText = strdup("I/O Timeout");
	else if (err == err_get_code(kIOReturnOffline))				errText = strdup("device offline");
	else if (err == err_get_code(kIOReturnNotReady))			errText = strdup("not ready");
	else if (err == err_get_code(kIOReturnNotAttached))			errText = strdup("device not attached");
	else if (err == err_get_code(kIOReturnNoChannels))			errText = strdup("no DMA channels left");
	else if (err == err_get_code(kIOReturnNoSpace))				errText = strdup("no space for data ");
	else if (err == err_get_code(kIOReturnPortExists))			errText = strdup("port already exists");
	else if (err == err_get_code(kIOReturnCannotWire))			errText = strdup("can't wire down physical memory");
	else if (err == err_get_code(kIOReturnNoInterrupt))			errText = strdup("no interrupt attached");
	else if (err == err_get_code(kIOReturnNoFrames))			errText = strdup("no DMA frames enqueued");
	else if (err == err_get_code(kIOReturnMessageTooLarge))		errText = strdup("oversized msg received on interrupt port");
	else if (err == err_get_code(kIOReturnNotPermitted))		errText = strdup("not permitted");
	else if (err == err_get_code(kIOReturnNoPower))				errText = strdup("no power to device");
	else if (err == err_get_code(kIOReturnNoMedia))				errText = strdup("media not present");
	else if (err == err_get_code(kIOReturnUnformattedMedia))	errText = strdup("edia not formatted");
	else if (err == err_get_code(kIOReturnUnsupportedMode))		errText = strdup("no such mode");
	else if (err == err_get_code(kIOReturnUnderrun))			errText = strdup("data underrun");
	else if (err == err_get_code(kIOReturnOverrun))				errText = strdup("data overrun");
	else if (err == err_get_code(kIOReturnDeviceError))			errText = strdup("the device is not working properly");
	else if (err == err_get_code(kIOReturnNoCompletion))		errText = strdup("a completion routine is required");
	else if (err == err_get_code(kIOReturnAborted))				errText = strdup("operation aborted");
	else if (err == err_get_code(kIOReturnNoBandwidth))			errText = strdup("bus bandwidth would be exceeded");
	else if (err == err_get_code(kIOReturnNotResponding))		errText = strdup("device not responding");
	else if (err == err_get_code(kIOReturnIsoTooOld))			errText = strdup("isochronous I/O request for distant past");
	else if (err == err_get_code(kIOReturnIsoTooNew))			errText = strdup("isochronous I/O request for distant future");
	else if (err == err_get_code(kIOReturnNotFound))			errText = strdup("data was not found");
	else if (err == err_get_code(kIOReturnInvalid))				errText = strdup("invalid");
	else
		asprintf(&errText, "unknown(0x%04x)", err);

	char *text;
	asprintf(&text, "%s %s: %s", sysText, subText, errText);
	free(sysText);
	free(subText);
	free(errText);
	return text;
}
