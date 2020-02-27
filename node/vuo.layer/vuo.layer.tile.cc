/**
 * @file
 * vuo.layer.tile node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <vector>

#include "node.h"

#include "VuoLayer.h"

VuoModuleMetadata({
	"title" : "Tile Layer",
	"keywords" : [
		"copy",
		"filter",
		"grid",
		"wrap",
		"infinite",
	],
	"version" : "1.1.0",
	"node" : {
		"exampleCompositions" : []
	}
});

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
	VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0},
							  "suggestedMin":{"x":-2.0,"y":-2.0},
							  "suggestedMax":{"x":2.0,"y":2.0},
							  "suggestedStep":{"x":0.1,"y":0.1}}) center,
	VuoInputData(VuoPoint2d, {"default":{"x":1.0,"y":1.0},
							  "suggestedMin":{"x":0.1,"y":0.1},
							  "suggestedMax":{"x":2.0,"y":2.0},
							  "suggestedStep":{"x":0.1,"y":0.1}}) spacing,
	VuoInputData(VuoPoint2d, {"default":{"x":2.0,"y":2.0},
							  "suggestedMin":{"x":0.0,"y":0.0},
							  "suggestedMax":{"x":10.0,"y":10.0},
							  "suggestedStep":{"x":0.1,"y":0.1}}) fieldSize,
//	VuoInputData(VuoBoolean, {"default":false}) reflectOddColumns,
//	VuoInputData(VuoBoolean, {"default":false}) reflectOddRows,
//	VuoInputData(VuoBoolean, {"default":false}) fadeBoundaryCopies,
	VuoOutputData(VuoLayer) tiledLayer
)
{
	*tiledLayer = (VuoLayer)VuoSceneObject_makeGroup(VuoListCreate_VuoSceneObject(), VuoTransform_makeIdentity());
	VuoLayer_setId(*tiledLayer, (*context)->groupID);

	VuoList_VuoSceneObject childObjects = VuoSceneObject_getChildObjects((VuoSceneObject)*tiledLayer);

	VuoPoint2d clampedFieldSize = VuoPoint2d_makeNonzero(fieldSize);
	VuoPoint2d clampedSpacing   = (VuoPoint2d){ MAX(0.01f, spacing.x), MAX(0.01f, spacing.y) };

	__block int idIndex = 0;
	for (float y = remainder(center.y, clampedSpacing.y) - clampedFieldSize.y / 2; y < (clampedFieldSize.y + clampedSpacing.y) / 2.; y += clampedSpacing.y)
	for (float x = remainder(center.x, clampedSpacing.x) - clampedFieldSize.x / 2; x < (clampedFieldSize.x + clampedSpacing.x) / 2.; x += clampedSpacing.x)
	{
		VuoTransform transform = VuoTransform_makeEuler((VuoPoint3d){x,y,0},
														(VuoPoint3d){0,0,0},
														(VuoPoint3d){1,1,1});

		VuoSceneObject so = VuoSceneObject_copy((VuoSceneObject)layer);
		VuoSceneObject_transform(so, transform);

		VuoSceneObject_apply(so, ^(VuoSceneObject current, float modelviewMatrix[16]){
			if ((*context)->copyIDs->size() <= idIndex)
				(*context)->copyIDs->push_back(VuoSceneObject_getNextId());
			VuoSceneObject_setId(current, (*(*context)->copyIDs)[idIndex++]);
		});

		VuoListAppendValue_VuoSceneObject(childObjects, so);
	}
}

extern "C" void nodeInstanceFini(
	VuoInstanceData(nodeInstanceData *) context)
{
	delete (*context)->copyIDs;
}
