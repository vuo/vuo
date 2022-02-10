/**
 * @file
 * vuo.image.get.cube node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoCubemap.h"

VuoModuleMetadata({
	"title" : "Get Cubemap Images",
	"keywords" : [
		"skybox", "environment map", "360", "180", "VR", "360vr",
	],
	"version" : "1.0.0",
	"node" : {
		"exampleCompositions" : [ ]
	}
});

void nodeEvent(
	VuoInputData(VuoCubemap) cubemap,
	VuoOutputData(VuoImage) front,
	VuoOutputData(VuoImage) left,
	VuoOutputData(VuoImage) right,
	VuoOutputData(VuoImage) back,
	VuoOutputData(VuoImage) top,
	VuoOutputData(VuoImage) bottom)
{
	*front  = VuoCubemap_getFront(cubemap);
	*left   = VuoCubemap_getLeft(cubemap);
	*right  = VuoCubemap_getRight(cubemap);
	*back   = VuoCubemap_getBack(cubemap);
	*top    = VuoCubemap_getTop(cubemap);
	*bottom = VuoCubemap_getBottom(cubemap);
}
