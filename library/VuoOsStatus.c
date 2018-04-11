/**
 * @file
 * VuoOsStatus implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoOsStatus.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

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
