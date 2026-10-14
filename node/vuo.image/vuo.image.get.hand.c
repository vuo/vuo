/**
 * @file
 * vuo.image.get.hand node implementation.
 *
 * @copyright Copyright © 2012–2026 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoHumanHand.h"

VuoModuleMetadata({
	"title" : "Get Hand Landmarks",
	"keywords" : [
		"humans", "person", "people",
		"left", "right", "side",
		"arms", "wrists", "handed", "fingers", "fingertips",
		"palms", "digits", "knuckles",
		"pointer", "pointable", "pinky", "opposable",
		"phalanges", "interphalangeal", "distal", "intermediate", "proximal", "metacarpophalangeal", "metacarpals", "carpometacarpal", "trapezium",
		"skeletal", "skeleton", "joints", "bones", "pose",
	],
	"version" : "1.0.0",
	"node" : {
		"exampleCompositions" : [ "CountWithFingers.vuo" ]
	}
});

void nodeEvent(
	VuoInputData(VuoHumanHand) hand,
	VuoOutputData(VuoRectangle) rectangle,
	VuoOutputData(VuoPoint2d) wrist,
	VuoOutputData(VuoList_VuoPoint2d) thumb,
	VuoOutputData(VuoList_VuoPoint2d) indexFinger,
	VuoOutputData(VuoList_VuoPoint2d) middleFinger,
	VuoOutputData(VuoList_VuoPoint2d) ringFinger,
	VuoOutputData(VuoList_VuoPoint2d) littleFinger
)
{
	struct json_object *h = (struct json_object *)hand;
	if (!h || !json_object_is_type(h, json_type_object))
	{
		*rectangle    = (VuoRectangle){{NAN, NAN}, {NAN, NAN}};
		*wrist        = (VuoPoint2d){NAN, NAN};
		*thumb        = NULL;
		*indexFinger  = NULL;
		*middleFinger = NULL;
		*ringFinger   = NULL;
		*littleFinger = NULL;
		return;
	}

	*rectangle    = VuoRectangle_makeFromJson(json_object_object_get(h, "rectangle"));
	*wrist        = VuoPoint2d_makeFromJson(json_object_object_get(h, "wrist"));
	*thumb        = VuoList_VuoPoint2d_makeFromJson(json_object_object_get(h, "thumb"));
	*indexFinger  = VuoList_VuoPoint2d_makeFromJson(json_object_object_get(h, "indexFinger"));
	*middleFinger = VuoList_VuoPoint2d_makeFromJson(json_object_object_get(h, "middleFinger"));
	*ringFinger   = VuoList_VuoPoint2d_makeFromJson(json_object_object_get(h, "ringFinger"));
	*littleFinger = VuoList_VuoPoint2d_makeFromJson(json_object_object_get(h, "littleFinger"));
}
