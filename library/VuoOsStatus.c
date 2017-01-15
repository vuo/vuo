/**
 * @file
 * VuoOsStatus implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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

	char *errorString = (char *)calloc(1, 7);

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
