/**
 * @file
 * vuo.scene.make.torus node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "VuoMeshParametric.h"

VuoModuleMetadata({
					  "title" : "Make Torus",
					  "keywords" : [ "mesh", "3d", "scene", "donut", "doughnut", "shape" ],
					  "version" : "1.1.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
						  }
					  },
					  "dependencies" : [ "VuoMeshParametric" ],
					  "node" : {
						  "exampleCompositions" : []
					  }
				  });

struct nodeInstanceData
{
	VuoInteger rows;
	VuoInteger columns;
	VuoReal thickness;
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
	VuoInputData(VuoInteger, {"default":16,"suggestedMin":3, "suggestedMax":256}) rows,
	VuoInputData(VuoInteger, {"default":32,"suggestedMin":3, "suggestedMax":256}) columns,
	VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.01}) thickness,
	VuoOutputData(VuoSceneObject) object
)
{
	if(VuoReal_areEqual(0, thickness))
	{
		*object = NULL;
		return;
	}

	// If the structure hasn't changed, just reuse the existing GPU mesh data.
	if (rows == (*context)->rows
	 && columns == (*context)->columns
	 && thickness == (*context)->thickness)
	{
		*object = VuoSceneObject_copy(*object);
		VuoSceneObject_setTransform(*object, transform);
		VuoSceneObject_setShader(*object, VuoShader_make_VuoGenericType1(material));
		return;
	}

	float _thickness = fmin(.2499, fmax(0, thickness * .25));
	float _diameter = .5 - _thickness;

	char* xExp = VuoText_format("(%f+%f*cos(v*360))*sin(u*360)", _diameter, _thickness);
	char* yExp = VuoText_format("(%f+%f*cos(v*360))*cos(u*360)", _diameter, _thickness);
	char* zExp = VuoText_format("-%f*sin(v*360)", _thickness);

	unsigned int r = MAX(3, MIN(511, rows)) + 1;
	unsigned int c = MAX(3, MIN(511, columns)) + 1;

	VuoMesh mesh = VuoMeshParametric_generate(	0,
												xExp, yExp, zExp,
												c, r,
												true,		// close u
												0, 1,
												true,		// close v
												0, 1,
												NULL);

	free(xExp);
	free(yExp);
	free(zExp);

	*object = VuoSceneObject_makeMesh(mesh, VuoShader_make_VuoGenericType1(material), transform);

	(*context)->rows = rows;
	(*context)->columns = columns;
	(*context)->thickness = thickness;
}
