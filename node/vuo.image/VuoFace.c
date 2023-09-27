/**
 * @file
 * VuoFace implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoFace.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title": "Face",
	"description": "Coordinates of a face and its landmarks.",
	"keywords": [],
	"version": "1.0.0",
	"dependencies" : [
		"VuoRectangle",
		"VuoPoint2d",
	]
});
#endif
/// @}

/**
 * @ingroup VuoFace
 * Decodes the JSON object `js` to create a new value.
 *
 * @eg{
 *   {
 *       "face": {"center":[0,0],"size":[1,1]},
 *       "leftEye": [0,0],
 *       "rightEye": [0,0],
 *       "nose": [0,0],
 *       "mouthLeftEdge": [0,0],
 *       "mouthRightEdge": [0,0],
 *   }
 * }
 */
VuoFace VuoFace_makeFromJson(json_object *js)
{
	return (VuoFace){
		VuoJson_getObjectValue(VuoRectangle, js, "face",           (VuoRectangle){{0,0},{0,0}}),
		VuoJson_getObjectValue(VuoPoint2d,   js, "leftEye",        (VuoPoint2d){0,0}),
		VuoJson_getObjectValue(VuoPoint2d,   js, "rightEye",       (VuoPoint2d){0,0}),
		VuoJson_getObjectValue(VuoPoint2d,   js, "nose",           (VuoPoint2d){0,0}),
		VuoJson_getObjectValue(VuoPoint2d,   js, "mouthLeftEdge",  (VuoPoint2d){0,0}),
		VuoJson_getObjectValue(VuoPoint2d,   js, "mouthRightEdge", (VuoPoint2d){0,0}),
	};
}

/**
 * @ingroup VuoFace
 * Encodes `f` as a JSON object.
 */
json_object *VuoFace_getJson(const VuoFace f)
{
	json_object *js = json_object_new_object();
	json_object_object_add(js, "face",           VuoRectangle_getJson(f.face));
	json_object_object_add(js, "leftEye",        VuoPoint2d_getJson(f.leftEye));
	json_object_object_add(js, "rightEye",       VuoPoint2d_getJson(f.rightEye));
	json_object_object_add(js, "nose",           VuoPoint2d_getJson(f.nose));
	json_object_object_add(js, "mouthLeftEdge",  VuoPoint2d_getJson(f.mouthLeftEdge));
	json_object_object_add(js, "mouthRightEdge", VuoPoint2d_getJson(f.mouthRightEdge));
	return js;
}

/**
 * @ingroup VuoFace
 * Returns a compact string representation of `f`.
 */
char *VuoFace_getSummary(const VuoFace f)
{
	return VuoRectangle_getSummary(f.face);
}

/**
 * @ingroup VuoFace
 * Creates a new face structure from the specified values.
 */
VuoFace VuoFace_make(VuoRectangle face,
					 VuoPoint2d leftEye,
					 VuoPoint2d rightEye,
					 VuoPoint2d nose,
					 VuoPoint2d mouthLeftEdge,
					 VuoPoint2d mouthRightEdge)
{
	return (VuoFace){
		face,
		leftEye,
		rightEye,
		nose,
		mouthLeftEdge,
		mouthRightEdge,
	};
}
