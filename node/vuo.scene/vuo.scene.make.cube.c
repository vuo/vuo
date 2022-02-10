/**
 * @file
 * vuo.scene.make.cube node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Make Cube with Materials",
					 "keywords" : [ "3D", "box", "d6", "hexahedron", "Platonic", "rectangular", "square", "shape", "object" ],
					 "version" : "1.1.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

struct nodeInstanceData
{
	VuoInteger rows;
	VuoInteger columns;
	VuoInteger slices;
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
		VuoInputData(VuoShader) frontShader,
		VuoInputData(VuoShader) leftShader,
		VuoInputData(VuoShader) rightShader,
		VuoInputData(VuoShader) backShader,
		VuoInputData(VuoShader) topShader,
		VuoInputData(VuoShader) bottomShader,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":2, "suggestedMax":256}) rows,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":2, "suggestedMax":256}) columns,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":2, "suggestedMax":256}) slices,
		VuoOutputData(VuoSceneObject) cube
)
{
	// If the structure hasn't changed, just reuse the existing GPU mesh data.
	if (rows == (*context)->rows
	 && columns == (*context)->columns
	 && slices == (*context)->slices)
	{
		*cube = VuoSceneObject_copy(*cube);

		VuoSceneObject_setTransform(*cube, transform);

		VuoSceneObject *objects = VuoListGetData_VuoSceneObject(VuoSceneObject_getChildObjects(*cube));
		VuoShader shaders[6] = {frontShader, leftShader, rightShader, backShader, topShader, bottomShader};
		for (int i = 0; i < 6; ++i)
			VuoSceneObject_setShader(objects[i], VuoShader_make_VuoShader(shaders[i]));

		return;
	}

	*cube = VuoSceneObject_makeCubeMulti(transform, columns, rows, slices, frontShader, leftShader, rightShader, backShader, topShader, bottomShader);
	VuoSceneObject_setName(*cube, VuoText_make("Cube"));

	(*context)->rows = rows;
	(*context)->columns = columns;
	(*context)->slices = slices;
}
