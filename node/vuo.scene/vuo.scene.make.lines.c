/**
 * @file
 * vuo.scene.make.lines node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title" : "Make Lines Object",
	"keywords" : [
		"segments", "points",
		"3D", "mesh", "model", "vertices", "shader", "texture", "draw", "opengl", "scenegraph", "graphics",
	],
	"version" : "1.0.0",
	"genericTypes" : {
		"VuoGenericType1": {
			"defaultType": 'VuoPoint2d',
			"compatibleTypes": [ "VuoPoint2d", "VuoPoint3d" ],
		},
		"VuoGenericType2" : {
			"compatibleTypes": [ "VuoShader", "VuoColor", "VuoImage" ],
		},
	},
	"node": {
		"exampleCompositions" : [ ],
	}
});

struct nodeInstanceData
{
	VuoList_VuoGenericType1 positions;
	VuoList_VuoColor colors;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	return context;
}

void nodeInstanceEvent(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoTransform) transform,
	VuoInputData(VuoGenericType2, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) material,
	VuoInputData(VuoReal, {"default":0.01, "suggestedMin":0.0, "suggestedStep":0.001}) lineWidth,
	VuoInputData(VuoList_VuoGenericType1, {"default":[[-0.5,0.5],[0,-0.5],[0,-0.5],[0.5,0.5]]}) positions,
	VuoInputData(VuoList_VuoColor, {"default":["#fff"]}) colors,
	VuoOutputData(VuoSceneObject) object)
{
	// If the structure hasn't changed, just reuse the existing GPU mesh data.
	if (VuoList_VuoGenericType1_areEqual(positions, (*context)->positions)
		&& VuoList_VuoColor_areEqual(colors, (*context)->colors))
	{
		*object = VuoSceneObject_copy(*object);
		VuoSceneObject_setTransform(*object, transform);
		VuoSceneObject_setShader(*object, VuoShader_make_VuoGenericType2(material));
		VuoMesh m = VuoMesh_copyShallow(VuoSceneObject_getMesh(*object));
		VuoMesh_setPrimitiveSize(m, lineWidth);
		VuoSceneObject_setMesh(*object, m);
		return;
	}

	if (VuoListGetCount_VuoGenericType1(positions) < 2)
	{
		*object = NULL;
		VuoRelease((*context)->positions);
		(*context)->positions = NULL;
		return;
	}

	VuoMesh mesh = VuoMesh_make_VuoGenericType1(positions, colors, VuoMesh_IndividualLines, lineWidth);
	*object = VuoSceneObject_makeMesh(mesh, VuoShader_make_VuoGenericType2(material), transform);

	VuoRetain(positions);
	VuoRelease((*context)->positions);
	(*context)->positions = positions;

	VuoRetain(colors);
	VuoRelease((*context)->colors);
	(*context)->colors = colors;
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) context)
{
	VuoRelease((*context)->positions);
	VuoRelease((*context)->colors);
}
