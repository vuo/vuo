/**
 * @file
 * vuo.syphon.filter.serverDescription.applicationName node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSyphon.h"

VuoModuleMetadata({
					  "title" : "Filter Server Descriptions by App",
					  "keywords" : [ "application", "frame", "input", "interprocess", "IOSurface", "name", "output", "receive", "send", "server", "share", "video" ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : false,
						  "exampleCompositions" : [ "ReceiveImagesPreferablyFromVuo.vuo" ]
					  },
					  "dependencies" : [
						  "VuoSyphon"
					  ]
				  });


void nodeEvent
(
		VuoInputData(VuoList_VuoSyphonServerDescription) serverDescriptions,
		VuoInputData(VuoText, {"default":""}) applicationName,
		VuoOutputData(VuoList_VuoSyphonServerDescription) filteredServerDescriptions
)
{
	VuoSyphonServerDescription partialDescription = VuoSyphonServerDescription_make(VuoText_make(""), VuoText_make(""), applicationName);
	*filteredServerDescriptions = VuoSyphon_filterServerDescriptions(serverDescriptions, partialDescription);
}
