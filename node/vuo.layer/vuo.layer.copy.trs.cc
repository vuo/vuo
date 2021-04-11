/**
 * @file
 * vuo.layer.copy.trs node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <vector>

#include "node.h"

extern "C" {
#include "VuoLayer.h"

VuoModuleMetadata({
	"title" : "Copy Layer (TRS)",
	"keywords" : [ "duplicate", "clone", "repeat", "replicate", "array", "instance", "instantiate", "populate" ],
	"version" : "2.1.0",
	"node" : {
		"exampleCompositions" : []
	}
});
}

#define DEG2RAD 0.0174532925

struct nodeInstanceData
{
	uint64_t groupID;
	std::vector<uint64_t> *copyIDs;
};

extern "C" nodeInstanceData *nodeInstanceInit(void)
{
	auto context = static_cast<nodeInstanceData *>(calloc(1, sizeof(nodeInstanceData)));
	VuoRegister(context, free);

	context->groupID = VuoSceneObject_getNextId();

	context->copyIDs = new std::vector<uint64_t>;

	return context;
}

extern "C" void nodeInstanceEvent(
	VuoInstanceData(nodeInstanceData *) context,
	VuoInputData(VuoLayer) layer,
	VuoInputData(VuoList_VuoPoint2d, { "default" : [ { "x" : 0, "y" : 0 } ] }) translations,
	VuoInputData(VuoList_VuoReal, { "default" : [0] }) rotations,
	VuoInputData(VuoList_VuoPoint2d, { "default" : [ { "x" : 1, "y" : 1 } ] }) scales,
	VuoOutputData(VuoLayer) copies)
{
	// get largest array ('cause we extrapolate if other arrays aren't equal in size to match largest)
	unsigned int t = VuoListGetCount_VuoPoint2d(translations),
				 r = VuoListGetCount_VuoReal(rotations),
				 s = VuoListGetCount_VuoPoint2d(scales);

	// If any list is empty, don't make any copies.
	if (t == 0 || r == 0 || s == 0)
	{
		*copies = nullptr;
		return;
	}

	unsigned int len = MAX(t, MAX(r, s));

	VuoList_VuoLayer layers = VuoListCreate_VuoLayer();

	__block int idIndex = 0;
	for(int i = 0; i < len; i++)
	{
		VuoPoint2d translation = VuoListGetValue_VuoPoint2d(translations, i + 1);
		VuoReal rotation       = VuoListGetValue_VuoReal(rotations, i + 1);
		VuoPoint2d scale       = VuoListGetValue_VuoPoint2d(scales, i + 1);

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

		VuoSceneObject so = VuoSceneObject_copy((VuoSceneObject)layer);
		VuoSceneObject_transform(so,
														 VuoTransform_makeEuler(
															 VuoPoint3d_make(translation.x, translation.y, 0),
															 VuoPoint3d_make(0, 0, rotation * DEG2RAD),
															 VuoPoint3d_make(scale.x, scale.y, 1)));

		VuoSceneObject_apply(so, ^(VuoSceneObject current, float modelviewMatrix[16]){
			if ((*context)->copyIDs->size() <= idIndex)
				(*context)->copyIDs->push_back(VuoSceneObject_getNextId());
			VuoSceneObject_setId(current, (*(*context)->copyIDs)[idIndex++]);
		});

		VuoListAppendValue_VuoLayer(layers, (VuoLayer)so);
	}

	VuoRetain(layers);
	*copies = VuoLayer_makeGroup(layers, VuoTransform2d_makeIdentity());
	VuoLayer_setId(*copies, (*context)->groupID);
	VuoRelease(layers);
}

extern "C" void nodeInstanceFini(
	VuoInstanceData(nodeInstanceData *) context)
{
	delete (*context)->copyIDs;
}
