/**
 * @file
 * vuo.scene.make.triangles node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMeshUtility.h"

VuoModuleMetadata({
	"title": "Make Triangles Object",
	"keywords": [
		"3-gon", "3gon", "shape", "polygon",
		"mesh", "model", "vertices", "shader", "texture", "draw", "opengl", "scenegraph", "graphics",
	],
	"version": "1.0.0",
	"dependencies" : [
		"VuoMeshUtility"
	],
	"genericTypes": {
		"VuoGenericType1": {
			"defaultType": 'VuoPoint2d',
			"compatibleTypes": [ "VuoPoint2d", "VuoPoint3d" ],
		},
		"VuoGenericType2" : {
			"compatibleTypes": [ "VuoShader", "VuoColor", "VuoImage" ],
		},
	},
	"node":{
		"exampleCompositions": [ ],
	},
});

struct nodeInstanceData
{
	VuoInteger textureMapping;
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
	VuoInputData(VuoInteger, {"default":0, "menuItems":[
		{"value": 0, "name": "Cubic"},
		{"value": 1, "name": "Spherical"},
	]}) textureMapping,
	VuoInputData(VuoList_VuoGenericType1, {"default":[[-0.5,-0.5],[0.5,-0.5],[0,0.5]]}) positions,
	VuoInputData(VuoList_VuoColor, {"default":["#fff"]}) colors,
	VuoOutputData(VuoSceneObject) object)
{
	// If the structure hasn't changed, just reuse the existing GPU mesh data.
	if (textureMapping == (*context)->textureMapping
	 && VuoList_VuoGenericType1_areEqual(positions, (*context)->positions)
	 && VuoList_VuoColor_areEqual(colors, (*context)->colors))
	{
		*object = VuoSceneObject_copy(*object);
		VuoSceneObject_setTransform(*object, transform);
		VuoSceneObject_setShader(*object, VuoShader_make_VuoGenericType2(material));
		return;
	}

	if (VuoListGetCount_VuoGenericType1(positions) < 2)
	{
		*object = NULL;
		return;
	}

	VuoMesh mesh = VuoMesh_make_VuoGenericType1(positions, colors, VuoMesh_IndividualTriangles, 0);
	if (!mesh)
		return;

	VuoMeshUtility_calculateNormals(mesh);

	if (textureMapping == 0)
		VuoMeshUtility_calculateCubicUVs(mesh);
	else // if (textureMapping == 1)
		VuoMeshUtility_calculateSphericalUVs(mesh);

	*object = VuoSceneObject_makeMesh(mesh, VuoShader_make_VuoGenericType2(material), transform);

	(*context)->textureMapping = textureMapping;

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
