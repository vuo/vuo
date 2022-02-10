/**
 * @file
 * vuo.transform.make.list.trs node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Transform List (TRS)",
					 "keywords" : [
						 "copy", "duplicate", "clone", "repeat", "replicate", "array", "instance", "instantiate", "populate",
						 "3D", "point",
						 "rotate", "pitch", "yaw", "roll", "euler", "angle",
					 ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoPoint3d, {"default":[{"x":0,"y":0,"z":0}]}) translations,
	VuoInputData(VuoList_VuoPoint3d, {"default":[{"x":0,"y":0,"z":0}]}) rotations,
	VuoInputData(VuoList_VuoPoint3d, {"default":[{"x":1,"y":1,"z":1}]}) scales,
	VuoOutputData(VuoList_VuoTransform) transforms
)
{
	/// @todo consider deduplicating with vuo.scene.copy.trs

	// get largest array (extrapolate if other arrays aren't big enough)
	unsigned int    t = VuoListGetCount_VuoPoint3d(translations),
					r = VuoListGetCount_VuoPoint3d(rotations),
					s = VuoListGetCount_VuoPoint3d(scales);

	// If any list is empty, don't make any copies.
	if (t == 0 || r == 0 || s == 0)
	{
		*transforms = NULL;
		return;
	}

	unsigned int len = MAX(t, MAX(r, s));

	*transforms = VuoListCreateWithCount_VuoTransform(len, VuoTransform_makeIdentity());
	VuoTransform *transformData = VuoListGetData_VuoTransform(*transforms);

	for (int i = 0; i < len; i++)
	{
		VuoPoint3d translation = VuoListGetValue_VuoPoint3d(translations, i+1);
		VuoPoint3d rotation = VuoListGetValue_VuoPoint3d(rotations, i+1);
		VuoPoint3d scale = VuoListGetValue_VuoPoint3d(scales, i+1);

		// if i is greater than the length of array, the value will be clamped to the last item in list.  use the last item and prior to last
		// item to linearly extrapolate the next value.
		if(i >= t)
			translation = VuoPoint3d_add(translation,
				VuoPoint3d_multiply(VuoPoint3d_subtract(translation, VuoListGetValue_VuoPoint3d(translations, t-1)), i-(t-1))
				);

		if(i >= r)
			rotation = VuoPoint3d_add(rotation,
				VuoPoint3d_multiply(VuoPoint3d_subtract(rotation, VuoListGetValue_VuoPoint3d(rotations, r-1)), i-(r-1))
				);

		if(i >= s)
			scale = VuoPoint3d_add(scale,
				VuoPoint3d_multiply(VuoPoint3d_subtract(scale, VuoListGetValue_VuoPoint3d(scales, s-1)), i-(s-1))
				);

		transformData[i] = VuoTransform_makeEuler(translation, VuoPoint3d_multiply(rotation, M_PI/180.), scale);
	}
}
