/**
 * @file
 * vuo.syphon.find.server.app node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSyphon.h"

VuoModuleMetadata({
	"title" : "Find Servers by App",
	"keywords" : [ "application", "frame", "input", "interprocess", "IOSurface", "name", "output", "share", "video", "filter", "search" ],
	"version" : "3.0.0",
	"node" : {
		"exampleCompositions" : ["ReceiveImagesPreferablyFromVuo.vuo"]
	},
	"dependencies" : ["VuoSyphon"]
});

void nodeEvent(
	VuoInputData(VuoList_VuoSyphonServerDescription, { "name" : "Servers" }) serverDescriptions,
	VuoInputData(VuoText, { "default" : "*" }) applicationName,
	VuoOutputData(VuoList_VuoSyphonServerDescription, { "name" : "Found Servers" }) foundServerDescriptions)
{
	VuoSyphonServerDescription partialDescription = VuoSyphonServerDescription_make(VuoText_make("*"), VuoText_make("*"), applicationName, true);
	VuoSyphonServerDescription_retain(partialDescription);
	*foundServerDescriptions = VuoSyphon_filterServerDescriptions(serverDescriptions, partialDescription);
	VuoSyphonServerDescription_release(partialDescription);
}
