/**
 * @file
 * VuoOsStatus implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoOsStatus.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <CoreVideo/CVReturn.h>
#include <CoreMediaIO/CMIOHardware.h>

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoOsStatus",
					  "dependencies" : [
						  "CoreFoundation.framework"
					  ]
				 });
#endif

/**
 * Returns a string providing a (slightly) better explanation of `error`.
 *
 * The caller is responsible for freeing the returned string.
 */
char *VuoOsStatus_getText(OSStatus error)
{
	if (error == 0)
		return strdup("(no error)");
	else if (error == bdNamErr)
		return strdup("Bad file name");
	else if (error == fnfErr)
		return strdup("File not found");

	// LSInfo.h
	else if (error == kLSAppInTrashErr)                              return strdup("The app cannot be run when inside a Trash folder");
	else if (error == kLSExecutableIncorrectFormat)                  return strdup("No compatible executable was found");
	else if (error == kLSAttributeNotFoundErr)                       return strdup("An item attribute value could not be found with the specified name");
	else if (error == kLSAttributeNotSettableErr)                    return strdup("The attribute is not settable");
	else if (error == kLSIncompatibleApplicationVersionErr)          return strdup("The app is incompatible with the current OS");
	else if (error == kLSNoRosettaEnvironmentErr)                    return strdup("The Rosetta environment was required not available");
	else if (error == -10666 /*kLSGarbageCollectionUnsupportedErr*/) return strdup("Objective-C garbage collection is no longer supported");
	else if (error == kLSUnknownErr)                                 return strdup("Unexpected internal error");
	else if (error == kLSNotAnApplicationErr)                        return strdup("Item needs to be an application, but is not");
	else if (error == kLSNotInitializedErr)                          return strdup("Not initialized");
	else if (error == kLSDataUnavailableErr)                         return strdup("Data unavailable (e.g., no kind string)");
	else if (error == kLSApplicationNotFoundErr)                     return strdup("Application not found (e.g., no application claims the file)");
	else if (error == kLSUnknownTypeErr)                             return strdup("Don't know anything about the type of the item");
	else if (error == kLSDataTooOldErr)                              return strdup("Data too old");
	else if (error == kLSDataErr)                                    return strdup("Data error");
	else if (error == kLSLaunchInProgressErr)                        return strdup("Attempted to launch an already launching application");
	else if (error == kLSNotRegisteredErr)                           return strdup("Not registered");
	else if (error == kLSAppDoesNotClaimTypeErr)                     return strdup("One or more documents are of types (and/or one or more URLs are of schemes) not supported by the target application");
	else if (error == kLSAppDoesNotSupportSchemeWarning)             return strdup("The app does not support this scheme");
	else if (error == kLSServerCommunicationErr)                     return strdup("The server process (registration and recent items) is not available");
	else if (error == kLSCannotSetInfoErr)                           return strdup("The extension visibility on this item cannot be changed");
	else if (error == kLSNoRegistrationInfoErr)                      return strdup("The item contains no registration info");
	else if (error == kLSIncompatibleSystemVersionErr)               return strdup("The app cannot run on the current OS version");
	else if (error == kLSNoLaunchPermissionErr)                      return strdup("User doesn't have permission to launch the app (managed networks)");
	else if (error == kLSNoExecutableErr)                            return strdup("The executable is missing");
	else if (error == kLSNoClassicEnvironmentErr)                    return strdup("The Classic environment was required but is not available");
	else if (error == kLSMultipleSessionsNotSupportedErr)            return strdup("The app cannot run simultaneously in two different sessions");

	// CVReturn.h
	else if (error == kCVReturnInvalidArgument)                         return strdup("At least one of the arguments passed in is not valid. Either out of range or the wrong type.");
	else if (error == kCVReturnAllocationFailed)                        return strdup("The allocation for a buffer or buffer pool failed. Most likely because of lack of resources.");
	else if (error == -6663 /*kCVReturnUnsupported*/)                   return strdup("Unsupported.");
	else if (error == kCVReturnInvalidDisplay)                          return strdup("A CVDisplayLink cannot be created for the given DisplayRef.");
	else if (error == kCVReturnDisplayLinkAlreadyRunning)               return strdup("The CVDisplayLink is already started and running.");
	else if (error == kCVReturnDisplayLinkNotRunning)                   return strdup("The CVDisplayLink has not been started.");
	else if (error == kCVReturnDisplayLinkCallbacksNotSet)              return strdup("The output callback is not set.");
	else if (error == kCVReturnInvalidPixelFormat)                      return strdup("The requested pixelformat is not supported for the CVBuffer type.");
	else if (error == kCVReturnInvalidSize)                             return strdup("The requested size (most likely too big) is not supported for the CVBuffer type.");
	else if (error == kCVReturnInvalidPixelBufferAttributes)            return strdup("A CVBuffer cannot be created with the given attributes.");
	else if (error == kCVReturnPixelBufferNotOpenGLCompatible)          return strdup("The Buffer cannot be used with OpenGL as either its size, pixelformat or attributes are not supported by OpenGL.");
	else if (error == -6684 /*kCVReturnPixelBufferNotMetalCompatible*/) return strdup("The Buffer cannot be used with Metal as either its size, pixelformat or attributes are not supported by Metal.");
	else if (error == kCVReturnWouldExceedAllocationThreshold)          return strdup("The allocation request failed because it would have exceeded a specified allocation threshold (see kCVPixelBufferPoolAllocationThresholdKey).");
	else if (error == kCVReturnPoolAllocationFailed)                    return strdup("The allocation for the buffer pool failed. Most likely because of lack of resources. Check if your parameters are in range.");
	else if (error == kCVReturnInvalidPoolAttributes)                   return strdup("A CVBufferPool cannot be created with the given attributes.");
	else if (error == -6692 /*kCVReturnRetry*/)                         return strdup("A scan hasn't completely traversed the CVBufferPool due to a concurrent operation. The client can retry the scan.");

	// CMIOHardware.h
	// AudioHardwareBase.h
	else if (error == kCMIOHardwareNotStoppedError)           return strdup("The function call requires that the hardware be stopped but it isn't.");
	else if (error == kCMIOHardwareNotRunningError)           return strdup("The function call requires that the hardware be running but it isn't.");
	else if (error == kCMIOHardwareUnspecifiedError)          return strdup("The function call failed while doing something that doesn't provide any error messages.");
	else if (error == kCMIOHardwareUnknownPropertyError)      return strdup("The object doesn't know about the property at the given address.");
	else if (error == kCMIOHardwareBadPropertySizeError)      return strdup("An improperly sized buffer was provided when accessing the data of a property.");
	else if (error == kCMIOHardwareIllegalOperationError)     return strdup("The requested operation couldn't be completed.");
	else if (error == kCMIOHardwareBadObjectError)            return strdup("The CMIOObjectID passed to the function doesn't map to a valid CMIOObject.");
	else if (error == kCMIOHardwareBadDeviceError)            return strdup("The CMIODeviceID passed to the function doesn't map to a valid CMIODevice.");
	else if (error == kCMIOHardwareBadStreamError)            return strdup("The CMIODeviceID passed to the function doesn't map to a valid CMIOStream.");
	else if (error == kCMIOHardwareUnsupportedOperationError) return strdup("The device doesn't support the requested operation.");
	else if (error == kCMIOHardwareSuspendedBySystemError)    return strdup("The function call failed because because access been suspended by the system.");
	else if (error == kCMIODeviceUnsupportedFormatError)      return strdup("The CMIOStream doesn't support the requested format.");
	else if (error == kCMIODevicePermissionsError)            return strdup("The requested operation can't be completed because the process doesn't have permission.");

	// AudioCodec.h
	else if (error == 561214580 /*kAudioCodecStateError*/)                return strdup("Codec state error.");
	else if (error == 560100710 /*kAudioCodecNotEnoughBufferSpaceError*/) return strdup("Not enough buffer space.");

	char *errorString = (char *)calloc(1, 12);  // ceil(log10(INT_MAX)) + 1 (optional negative sign) + 1 (null terminator)

	*(UInt32 *)(errorString + 1) = CFSwapInt32HostToBig(error);
	if (isprint(errorString[1]) && isprint(errorString[2]) && isprint(errorString[3]) && isprint(errorString[4]))
	{
		errorString[0] = errorString[5] = '\'';
		errorString[6] = '\0';
	}
	else
		sprintf(errorString, "%d", (int)error);

	return errorString;
}
