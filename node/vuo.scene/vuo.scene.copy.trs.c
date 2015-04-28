/**
 * @file
 * vuo.scene.copy.trs node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Copy 3D Object",
					 "keywords" : [ "duplicate", "array", "instance", "instantiate", "populate", "replicate" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false,
						 "exampleCompositions" : [ "DisplayRowOfSpheres.vuo" ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoSceneObject) object,
	VuoInputData(VuoList_VuoPoint3d) translations,
	VuoInputData(VuoList_VuoPoint3d) rotations,
	VuoInputData(VuoList_VuoPoint3d) scales,
	VuoOutputData(VuoList_VuoSceneObject) copies
)
{
	// get largest array (extrapolate if other arrays aren't big enough)
	unsigned int 	t = VuoListGetCount_VuoPoint3d(translations),
					r = VuoListGetCount_VuoPoint3d(rotations),
					s = VuoListGetCount_VuoPoint3d(scales);

	// I wonder what the most succinct way of writing this would be?
	unsigned int len = t;
	if(len < r || len < s)
		len = r > s ? r : s;

	*copies = VuoListCreate_VuoSceneObject();
	for(int i = 0; i < len; i++)
	{
		VuoPoint3d translation = VuoListGetValueAtIndex_VuoPoint3d(translations, i+1);
		VuoPoint3d rotation = VuoListGetValueAtIndex_VuoPoint3d(rotations, i+1);
		VuoPoint3d scale = VuoListGetValueAtIndex_VuoPoint3d(scales, i+1);

		// if i is greater than the length of array, the value will be clamped to the last item in list.  use the last item and prior to last
		// item to linearly extrapolate the next value.
		if(i >= t)
			translation = VuoPoint3d_add(translation,
				VuoPoint3d_multiply(VuoPoint3d_subtract(translation, VuoListGetValueAtIndex_VuoPoint3d(translations, t-1)), i-(t-1))
				);

		if(i >= r)
			rotation = VuoPoint3d_add(rotation,
				VuoPoint3d_multiply(VuoPoint3d_subtract(rotation, VuoListGetValueAtIndex_VuoPoint3d(rotations, r-1)), i-(r-1))
				);

		if(i >= s)
			scale = VuoPoint3d_add(scale,
				VuoPoint3d_multiply(VuoPoint3d_subtract(scale, VuoListGetValueAtIndex_VuoPoint3d(scales, s-1)), i-(s-1))
				);

		// VuoTransform_makeEuler(VuoPoint3d translation, VuoPoint3d rotation, VuoPoint3d scale)
		VuoListAppendValue_VuoSceneObject(*copies, VuoSceneObject_make(
			object.verticesList,
			object.shader,
			VuoTransform_makeEuler(translation, rotation, scale),
			object.childObjects
			));
	}
}
