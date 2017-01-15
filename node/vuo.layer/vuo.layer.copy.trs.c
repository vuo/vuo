/**
 * @file
 * vuo.layer.copy.trs node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Copy Layer",
					 "keywords" : [ "duplicate", "clone", "array", "instance", "instantiate", "populate" ],
					 "version" : "2.0.1",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

#define DEG2RAD 0.0174532925

void nodeEvent
(
	VuoInputData(VuoLayer) layer,
	VuoInputData(VuoList_VuoPoint2d, {"default":[{"x":0,"y":0}]}) translations,
	VuoInputData(VuoList_VuoReal, {"default":[0]}) rotations,
	VuoInputData(VuoList_VuoPoint2d, {"default":[{"x":1,"y":1}]}) scales,
	VuoOutputData(VuoLayer) copies
)
{
	// get largest array ('cause we extrapolate if other arrays aren't equal in size to match largest)
	unsigned int 	t = VuoListGetCount_VuoPoint2d(translations),
					r = VuoListGetCount_VuoReal(rotations),
					s = VuoListGetCount_VuoPoint2d(scales);

	// If any list is empty, don't make any copies.
	if (t == 0 || r == 0 || s == 0)
	{
		*copies = VuoLayer_makeEmpty();
		return;
	}

	unsigned int len = t;
	if(len < r || len < s) len = r > s ? r : s;

	VuoList_VuoLayer layers = VuoListCreate_VuoLayer();

	for(int i = 0; i < len; i++)
	{
		VuoPoint2d translation 	= VuoListGetValue_VuoPoint2d(translations, i+1);
		VuoReal rotation 		= VuoListGetValue_VuoReal(rotations, i+1);
		VuoPoint2d scale 		= VuoListGetValue_VuoPoint2d(scales, i+1);

		// if i is greater than the length of array, the value will be clamped to the last item in list.  use the last item and prior to last
		// item to linearly extrapolate the next value.
		if(i >= t)
			translation = VuoPoint2d_add(translation,
				VuoPoint2d_multiply(VuoPoint2d_subtract(translation, VuoListGetValue_VuoPoint2d(translations, t-1)), i-(t-1))
				);

		if(i >= r)
			rotation = ( rotation + (rotation - VuoListGetValue_VuoReal(rotations, r-1)) * (i-(r-1)) );

		if(i >= s)
			scale = VuoPoint2d_add(scale,
				VuoPoint2d_multiply(VuoPoint2d_subtract(scale, VuoListGetValue_VuoPoint2d(scales, s-1)), i-(s-1))
				);

		VuoLayer o;
		o.sceneObject = layer.sceneObject;
		o.sceneObject.transform = VuoTransform_composite(o.sceneObject.transform,
														 VuoTransform_makeEuler(
															 VuoPoint3d_make(translation.x, translation.y, 0),
															 VuoPoint3d_make(0, 0, rotation * DEG2RAD),
															 VuoPoint3d_make(scale.x, scale.y, 1)));

		VuoListAppendValue_VuoLayer(layers, o);
	}

	VuoRetain(layers);
	*copies = VuoLayer_makeGroup(layers, VuoTransform2d_makeIdentity());
	VuoRelease(layers);
}
