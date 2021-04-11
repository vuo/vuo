/**
 * @file
 * vuo.image.make.cube node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoCubemap.h"

VuoModuleMetadata({
	"title" : "Make Cubemap",
	"keywords" : [
		"skybox", "environment map", "360", "180", "VR", "360vr",
	],
	"version" : "1.0.0",
	"node" : {
		"exampleCompositions" : [ ]
	}
});

void nodeEvent(
	VuoInputData(VuoImage) front,
	VuoInputData(VuoImage) left,
	VuoInputData(VuoImage) right,
	VuoInputData(VuoImage) back,
	VuoInputData(VuoImage) top,
	VuoInputData(VuoImage) bottom,
	VuoOutputData(VuoCubemap) cubemap)
{
	*cubemap = VuoCubemap_makeFromImages(front, left, right, back, top, bottom);
}
