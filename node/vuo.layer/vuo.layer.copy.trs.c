/**
 * @file
 * vuo.layer.copy.trs node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Copy Layer",
					 "keywords" : [ "duplicate", "array", "instance", "instantiate", "populate" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

#define DEG2RAD 0.0174532925

void nodeEvent
(
	VuoInputData(VuoLayer) layer,
	VuoInputData(VuoList_VuoPoint2d) translations,
	VuoInputData(VuoList_VuoReal) rotations,
	VuoInputData(VuoList_VuoPoint2d) scales,
	VuoOutputData(VuoList_VuoLayer) copies
)
{
	// get largest array ('cause we extrapolate if other arrays aren't equal in size to match largest)
	unsigned int 	t = VuoListGetCount_VuoPoint2d(translations), 
					r = VuoListGetCount_VuoReal(rotations),
					s = VuoListGetCount_VuoPoint2d(scales);

	unsigned int len = t;
	if(len < r || len < s) len = r > s ? r : s;

	*copies = VuoListCreate_VuoLayer();

	for(int i = 0; i < len; i++)
	{
		VuoPoint2d translation 	= VuoListGetValueAtIndex_VuoPoint2d(translations, i+1);
		VuoReal rotation 		= VuoListGetValueAtIndex_VuoReal(rotations, i+1);
		VuoPoint2d scale 		= VuoListGetValueAtIndex_VuoPoint2d(scales, i+1);

		// if i is greater than the length of array, the value will be clamped to the last item in list.  use the last item and prior to last
		// item to linearly extrapolate the next value.
		if(i >= t)
			translation = VuoPoint2d_add(translation, 
				VuoPoint2d_multiply(VuoPoint2d_subtract(translation, VuoListGetValueAtIndex_VuoPoint2d(translations, t-1)), i-(t-1))
				);

		if(i >= r)
			rotation = ( rotation + (rotation - VuoListGetValueAtIndex_VuoReal(rotations, r-1)) * (i-(r-1)) );

		if(i >= s)
			scale = VuoPoint2d_add(scale, 
				VuoPoint2d_multiply(VuoPoint2d_subtract(scale, VuoListGetValueAtIndex_VuoPoint2d(scales, s-1)), i-(s-1))
				);

		VuoLayer o;
		o.sceneObject = layer.sceneObject;
		o.sceneObject.transform = VuoTransform_makeEuler(
									VuoPoint3d_make(translation.x, translation.y, 0),
									VuoPoint3d_make(0, 0, rotation * DEG2RAD),
									VuoPoint3d_make(scale.x, scale.y, 1));

		VuoListAppendValue_VuoLayer(*copies, o);
	}
}
