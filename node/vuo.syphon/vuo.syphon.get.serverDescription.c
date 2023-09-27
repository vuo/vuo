/**
 * @file
 * vuo.syphon.get.serverDescription node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSyphon.h"

VuoModuleMetadata({
					  "title" : "Get Syphon Server Values",
					  "keywords" : [ "application", "frame", "input", "interprocess", "IOSurface", "output", "share", "video" ],
					  "version" : "1.0.1",
					  "node": {
						  "exampleCompositions" : [ ]
					  },
					  "dependencies" : [
						"VuoSyphon"
					  ]
				  });


void nodeEvent
(
	VuoInputData(VuoSyphonServerDescription, {"name":"Server"}) serverDescription,
	VuoOutputData(VuoText) serverName,
	VuoOutputData(VuoText) applicationName
)
{
	*serverName = serverDescription.serverName;
	*applicationName = serverDescription.applicationName;
}
