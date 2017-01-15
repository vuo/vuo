/**
 * @file
 * vuo.syphon.find.server.name node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSyphon.h"

VuoModuleMetadata({
					  "title" : "Find Servers by Name",
					  "keywords" : [ "application", "frame", "input", "interprocess", "IOSurface", "output", "share", "video", "filter" ],
					  "version" : "2.0.0",
					  "node": {
						  "exampleCompositions" : [ "ReceiveImagesPreferablyFromVuo.vuo" ]
					  },
					  "dependencies" : [
						  "VuoSyphon"
					  ]
				  });


void nodeEvent
(
		VuoInputData(VuoList_VuoSyphonServerDescription) serverDescriptions,
		VuoInputData(VuoText, {"default":""}) serverName,
		VuoOutputData(VuoList_VuoSyphonServerDescription) foundServerDescriptions
)
{
	VuoSyphonServerDescription partialDescription = VuoSyphonServerDescription_make(VuoText_make(""), serverName, VuoText_make(""));
	VuoSyphonServerDescription_retain(partialDescription);
	*foundServerDescriptions = VuoSyphon_filterServerDescriptions(serverDescriptions, partialDescription);
	VuoSyphonServerDescription_release(partialDescription);
}
