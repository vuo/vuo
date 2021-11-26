/**
 * @file
 * vuo.scene.make.points node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMeshParametric.h"

VuoModuleMetadata({
	"title" : "Make Parametric Object",
	"keywords" : [
		"segments", "points",
		"3D", "mesh", "model", "vertices", "shader", "texture", "draw", "opengl", "scenegraph", "graphics",
	],
	"version" : "1.0.1",
	"dependencies" : [
		"VuoMeshParametric",
	],
	"genericTypes" : {
		"VuoGenericType1" : {
			"compatibleTypes": [ "VuoShader", "VuoColor", "VuoImage" ],
		},
	},
	"node": {
		"exampleCompositions" : [ "SpinShell.vuo" ],
	}
});

struct nodeInstanceData
{
	VuoReal time;
	VuoText xExpression;
	VuoText yExpression;
	VuoText zExpression;
	VuoInteger rows;
	VuoInteger columns;
	VuoBoolean uClosed;
	VuoRange uRange;
	VuoReal uMax;
	VuoBoolean vClosed;
	VuoRange vRange;
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
	VuoInputData(VuoGenericType1, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) material,
	VuoInputData(VuoReal, {"default":0.0}) time,
	VuoInputData(VuoText, {"default":"U"}) xExpression,
	VuoInputData(VuoText, {"default":"V + sin(U*360 + time*16)/4"}) yExpression,
	VuoInputData(VuoText, {"default":"0"}) zExpression,
	VuoInputData(VuoInteger, {"default":16,"suggestedMin":2,"suggestedMax":256}) rows,
	VuoInputData(VuoInteger, {"default":16,"suggestedMin":2,"suggestedMax":256}) columns,
	VuoInputData(VuoBoolean, {"default":false}) uClosed,
	VuoInputData(VuoRange, {"default":{"minimum":-0.5, "maximum":0.5},
		"requireMin":true,
		"requireMax":true,
		"suggestedMin":{"minimum":-2.0,"maximum":-2.0},
		"suggestedMax":{"minimum":2.0,"maximum":2.0},
		"suggestedStep":{"minimum":0.1,"maximum":0.1}}) uRange,
	VuoInputData(VuoBoolean, {"default":false}) vClosed,
	VuoInputData(VuoRange, {"default":{"minimum":-0.5, "maximum":0.5},
		"requireMin":true,
		"requireMax":true,
		"suggestedMin":{"minimum":-2.0,"maximum":-2.0},
		"suggestedMax":{"minimum":2.0,"maximum":2.0},
		"suggestedStep":{"minimum":0.1,"maximum":0.1}}) vRange,
	VuoOutputData(VuoSceneObject) object)
{
	// If the structure hasn't changed, just reuse the existing GPU mesh data.
	if (VuoReal_areEqual(time,        (*context)->time)
	 && VuoText_areEqual(xExpression, (*context)->xExpression)
	 && VuoText_areEqual(yExpression, (*context)->yExpression)
	 && VuoText_areEqual(zExpression, (*context)->zExpression)
	 && VuoInteger_areEqual(rows,     (*context)->rows)
	 && VuoInteger_areEqual(columns,  (*context)->columns)
	 && VuoBoolean_areEqual(uClosed,  (*context)->uClosed)
	 && VuoRange_areEqual(uRange,     (*context)->uRange)
	 && VuoBoolean_areEqual(vClosed,  (*context)->vClosed)
	 && VuoRange_areEqual(vRange,     (*context)->vRange))
	{
		*object = VuoSceneObject_copy(*object);
		VuoSceneObject_setTransform(*object, transform);
		VuoSceneObject_setShader(*object, VuoShader_make_VuoGenericType1(material));
		return;
	}

	VuoMesh mesh = VuoMeshParametric_generate(time, xExpression, yExpression, zExpression, columns, rows, (bool)uClosed, uRange.minimum, uRange.maximum, (bool)vClosed, vRange.minimum, vRange.maximum, NULL);
	*object = VuoSceneObject_makeMesh(mesh, VuoShader_make_VuoGenericType1(material), transform);


	(*context)->time = time;
	(*context)->rows = rows;
	(*context)->columns = columns;
	(*context)->uClosed = uClosed;
	(*context)->uRange = uRange;
	(*context)->vClosed = vClosed;
	(*context)->vRange = vRange;

	VuoRetain(xExpression);
	VuoRelease((*context)->xExpression);
	(*context)->xExpression = xExpression;

	VuoRetain(yExpression);
	VuoRelease((*context)->yExpression);
	(*context)->yExpression = yExpression;

	VuoRetain(zExpression);
	VuoRelease((*context)->zExpression);
	(*context)->zExpression = zExpression;
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) context)
{
	VuoRelease((*context)->xExpression);
	VuoRelease((*context)->yExpression);
	VuoRelease((*context)->zExpression);
}
