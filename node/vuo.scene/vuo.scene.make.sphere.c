/**
 * @file
 * vuo.scene.make.sphere node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "VuoMeshParametric.h"

VuoModuleMetadata({
					  "title" : "Make Sphere",
					  "keywords" : [ "mesh", "3d", "scene", "sphere", "ball", "round", "ellipsoid", "circle", "globe", "shape" ],
					  "version" : "1.1.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
						  }
					  },
					  "dependencies" : [ "VuoMeshParametric" ],
					  "node" : {
						  "exampleCompositions" : [ "DisplaySphere.vuo", "MoveBeadsOnString.vuo" ]
					  }
				  });

static const char *xExp = "sin((u-.5)*360) * cos((v-.5)*180) / 2.";
static const char *yExp = "sin((v-.5)*180) / 2.";
static const char *zExp = "cos((u-.5)*360) * cos((v-.5)*180) / 2.";

struct nodeInstanceData
{
	VuoInteger rows;
	VuoInteger columns;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	return context;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoTransform) transform,
	VuoInputData(VuoGenericType1, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) material,
	VuoInputData(VuoInteger, {"default":32,"suggestedMin":4, "suggestedMax":256}) rows,
	VuoInputData(VuoInteger, {"default":32,"suggestedMin":4, "suggestedMax":256}) columns,
	VuoOutputData(VuoSceneObject) object
)
{
	// If the structure hasn't changed, just reuse the existing GPU mesh data.
	if (rows == (*context)->rows
	 && columns == (*context)->columns)
	{
		*object = VuoSceneObject_copy(*object);
		VuoSceneObject_setTransform(*object, transform);
		VuoSceneObject_setShader(*object, VuoShader_make_VuoGenericType1(material));
		return;
	}

	unsigned int r = MAX(4, MIN(512, rows));
	unsigned int c = MAX(4, MIN(512, columns));

	VuoMesh mesh = VuoMeshParametric_generate(	0,
												xExp, yExp, zExp,
												c, r,
												true,		// close u
												0, 1,
												false,		// close v
												0, 1,
												NULL);

	*object = VuoSceneObject_makeMesh(mesh, VuoShader_make_VuoGenericType1(material), transform);
	VuoSceneObject_setName(*object, VuoText_make("Sphere"));

	(*context)->rows = rows;
	(*context)->columns = columns;
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) context
)
{
}
