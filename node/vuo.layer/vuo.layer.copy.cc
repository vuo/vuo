/**
 * @file
 * vuo.layer.copy node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <vector>

#include "VuoList_VuoTransform2d.h"

extern "C" {
#include "VuoLayer.h"

VuoModuleMetadata({
	"title" : "Copy Layer (Transform)",
	"keywords" : [ "duplicate", "clone", "repeat", "replicate", "array", "instance", "instantiate", "populate" ],
	"version" : "2.1.0",
	"node" : {
		"exampleCompositions" : []
	}
});
}

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
	VuoInputData(VuoList_VuoTransform2d) transforms,
	VuoOutputData(VuoLayer) copies)
{
	VuoList_VuoLayer layers = VuoListCreate_VuoLayer();
	unsigned long transform_length = VuoListGetCount_VuoTransform2d(transforms);

	__block int idIndex = 0;
	for (int i = 0; i < transform_length; ++i)
	{
		VuoSceneObject so = VuoSceneObject_copy((VuoSceneObject)layer);
		VuoSceneObject_transform(so,
			VuoTransform_makeFrom2d(VuoListGetValue_VuoTransform2d(transforms, i + 1)));

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
