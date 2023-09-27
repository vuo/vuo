/**
 * @file
 * vuo.syphon.make.serverDescription node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSyphon.h"
#include "VuoSyphonServerDescription.h"

VuoModuleMetadata({
					  "title" : "Make Syphon Server",
					  "keywords" : [ "application", "frame", "input", "interprocess", "IOSurface", "output", "share", "video" ],
					  "version" : "1.0.0",
					  "node": {
						  "isDeprecated": true,
						  "exampleCompositions" : [ "ReceiveImagesOnlyFromVuo.vuo" ]
					  },
					  "dependencies" : [
						"VuoSyphon"
					  ]
				  });


void nodeEvent
(
	VuoInputData(VuoText) serverName,
	VuoInputData(VuoText) applicationName,
	VuoOutputData(VuoSyphonServerDescription, {"name":"Server"}) serverDescription
)
{
	(*serverDescription) = VuoSyphonServerDescription_make(VuoText_make(""), serverName, applicationName, false);
}
