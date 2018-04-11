/**
 * @file
 * vuo.scene.copy.trs.material node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoExtrapolationMode.h"

VuoModuleMetadata({
					 "title" : "Copy 3D Object (TRS + Material)",
					 "keywords" : [
						 "duplicate", "clone", "repeat", "replicate", "array", "instance", "instantiate", "populate",
						 "shaders", "colors", "images",
					 ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "DisplayRowOfSpheres.vuo" ]
					 },
					 "genericTypes" : {
						 "VuoGenericType1" : {
							 "compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
						 }
					 },
				 });

void nodeEvent
(
	VuoInputData(VuoSceneObject) object,
	VuoInputData(VuoList_VuoGenericType1) materials,
	VuoInputData(VuoExtrapolationMode, {"default":"wrap"}) materialExtrapolation,
	VuoInputData(VuoList_VuoPoint3d, {"default":[{"x":0,"y":0,"z":0}]}) translations,
	VuoInputData(VuoList_VuoPoint3d, {"default":[{"x":0,"y":0,"z":0}]}) rotations,
	VuoInputData(VuoList_VuoPoint3d, {"default":[{"x":1,"y":1,"z":1}]}) scales,
	VuoOutputData(VuoSceneObject) copies
)
{
	// get largest array (extrapolate if other arrays aren't big enough)
	unsigned int 	m = VuoListGetCount_VuoGenericType1(materials),
					t = VuoListGetCount_VuoPoint3d(translations),
					r = VuoListGetCount_VuoPoint3d(rotations),
					s = VuoListGetCount_VuoPoint3d(scales);

	// If any list is empty, don't make any copies.
	if (m == 0 || t == 0 || r == 0 || s == 0)
	{
		*copies = VuoSceneObject_makeEmpty();
		return;
	}

	unsigned int len = MAX(m, MAX(t, MAX(r, s)));

	*copies = VuoSceneObject_makeGroup(VuoListCreate_VuoSceneObject(), VuoTransform_makeIdentity());

	long priorMaterialIndex = 0;
	VuoSceneObject objectWithNewShader;
	for (int i = 0; i < len; i++)
	{
		long materialIndex;
		if (materialExtrapolation == VuoExtrapolationMode_Wrap)
			materialIndex = (i % m) + 1;
		else // VuoExtrapolationMode_Stretch
			materialIndex = ((float)i / len) * m + 1;

		// Reuse the prior shader if possible.
		if (materialIndex != priorMaterialIndex)
		{
			VuoShader shader = VuoShader_make_VuoGenericType1(VuoListGetValue_VuoGenericType1(materials, materialIndex));
			objectWithNewShader = VuoSceneObject_copy(object);
			VuoSceneObject_apply(&objectWithNewShader, ^(VuoSceneObject *currentObject, float modelviewMatrix[16]){
									currentObject->shader = shader;
								 });

			priorMaterialIndex = materialIndex;
		}

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

		objectWithNewShader.transform = VuoTransform_composite(object.transform,
			VuoTransform_makeEuler(translation, VuoPoint3d_multiply(rotation, M_PI/180.), scale));

		VuoListAppendValue_VuoSceneObject(copies->childObjects, objectWithNewShader);
	}
}
