/**
 * @file
 * vuo.syphon.make.serverDescription node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSyphon.h"
#include "VuoSyphonServerDescription.h"

VuoModuleMetadata({
					  "title" : "Make Server Description",
					  "keywords" : [ "application", "frame", "input", "interprocess", "IOSurface", "output", "share", "video" ],
					  "version" : "1.0.0",
					  "node": {
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
	VuoOutputData(VuoSyphonServerDescription) serverDescription
)
{
	(*serverDescription) = VuoSyphonServerDescription_make(VuoText_make(""), serverName, applicationName);
}

