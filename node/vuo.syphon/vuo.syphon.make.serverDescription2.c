/**
 * @file
 * vuo.syphon.make.serverDescription node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoSyphon.h"
#include "VuoSyphonServerDescription.h"

VuoModuleMetadata({
	"title" : "Specify Syphon Server",
	"keywords" : [ "application", "frame", "input", "interprocess", "IOSurface", "output", "share", "video" ],
	"version" : "2.0.0",
	"node" : {
		"exampleCompositions" : ["ReceiveImagesOnlyFromVuo.vuo"]
	},
	"dependencies" : ["VuoSyphon"]
});

void nodeEvent(
	VuoInputData(VuoText, { "default" : "*" }) serverName,
	VuoInputData(VuoText, { "default" : "*" }) applicationName,
	VuoOutputData(VuoSyphonServerDescription, { "name" : "Server" }) serverDescription)
{
	(*serverDescription) = VuoSyphonServerDescription_make(VuoText_make("*"), serverName, applicationName, true);
}
