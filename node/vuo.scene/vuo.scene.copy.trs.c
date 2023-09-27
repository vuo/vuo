/**
 * @file
 * vuo.scene.copy.trs node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Copy 3D Object (TRS)",
					 "keywords" : [ "duplicate", "clone", "repeat", "replicate", "array", "instance", "instantiate", "populate" ],
					 "version" : "2.0.2",
					 "node": {
						 "exampleCompositions" : [ "DisplayRowOfSpheres.vuo" ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoSceneObject) object,
	VuoInputData(VuoList_VuoPoint3d, {"default":[{"x":0,"y":0,"z":0}]}) translations,
	VuoInputData(VuoList_VuoPoint3d, {"default":[{"x":0,"y":0,"z":0}]}) rotations,
	VuoInputData(VuoList_VuoPoint3d, {"default":[{"x":1,"y":1,"z":1}]}) scales,
	VuoOutputData(VuoSceneObject) copies
)
{
	// get largest array (extrapolate if other arrays aren't big enough)
	unsigned int 	t = VuoListGetCount_VuoPoint3d(translations),
					r = VuoListGetCount_VuoPoint3d(rotations),
					s = VuoListGetCount_VuoPoint3d(scales);

	// If any list is empty, don't make any copies.
	if (t == 0 || r == 0 || s == 0)
	{
		*copies = NULL;
		return;
	}

	unsigned int len = MAX(t, MAX(r, s));

	*copies = VuoSceneObject_makeGroup(VuoListCreate_VuoSceneObject(), VuoTransform_makeIdentity());

	for(int i = 0; i < len; i++)
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

		// VuoTransform_makeEuler(VuoPoint3d translation, VuoPoint3d rotation, VuoPoint3d scale)
		VuoSceneObject so = VuoSceneObject_copy(object);
		VuoSceneObject_transform(so, VuoTransform_makeEuler(translation, VuoPoint3d_multiply(rotation, M_PI/180.), scale));
		VuoListAppendValue_VuoSceneObject(VuoSceneObject_getChildObjects(*copies), so);
	}
}
