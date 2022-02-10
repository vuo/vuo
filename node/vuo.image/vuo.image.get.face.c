/**
 * @file
 * vuo.image.get.face node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoFace.h"

VuoModuleMetadata({
	"title" : "Get Face Landmarks",
	"keywords" : [
		"eyebrows", "forehead",
		"cheeks",
		"lips", "chin",
	],
	"version" : "1.0.0",
	"node" : {
		"exampleCompositions" : [ ]
	}
});

void nodeEvent(
	VuoInputData(VuoFace) landmarks,
	VuoOutputData(VuoRectangle) face,
	VuoOutputData(VuoPoint2d) leftEye,
	VuoOutputData(VuoPoint2d) rightEye,
	VuoOutputData(VuoPoint2d) nose,
	VuoOutputData(VuoPoint2d) mouthLeftEdge,
	VuoOutputData(VuoPoint2d) mouthRightEdge)
{
	*face           = landmarks.face;
	*leftEye        = landmarks.leftEye;
	*rightEye       = landmarks.rightEye;
	*nose           = landmarks.nose;
	*mouthLeftEdge  = landmarks.mouthLeftEdge;
	*mouthRightEdge = landmarks.mouthRightEdge;
}
