/**
 * @file
 * vuo.scene.make.supershape node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "VuoMeshParametric.h"

VuoModuleMetadata({
	"title" : "Make Supershape",
	"keywords" : [
		"mesh", "3d", "scene",
		"parametric", "shape",
		"natural", "organic", "symmetry",
		"morph", "animate",
		"circle", "circular", "oval", "polar",
		"sphere", "spherical",
		"blob",
		"aquatic", "starfish",
		"starfruit", "carambola",
		"flower", "floral", "cactus",
		"bundt cake",
		"lobes", "sectors", "bulges", "hollows", "corners",
		"pinched", "relaxed", "smooth",
		"Johan Gielis", "superformula", "formula",
		"Piet Hein", "superellipse", "ellipse",
	],
	"version" : "1.0.0",
	"genericTypes" : {
		"VuoGenericType1" : {
			"defaultType" : "VuoColor",
			"compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
		}
	},
	"dependencies" : [ "VuoMeshParametric" ],
	"node" : {
		"exampleCompositions" : [ "AnimateSupershape.vuo" ]
	}
});

struct nodeInstanceData
{
	double m;
	double n1;
	double n2;
	double n3;
	double a;
	double b;
	VuoRange angle;
	VuoRange radius;
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
	VuoInputData(VuoReal, {"name":"m",  "default":8, "suggestedMin":0, "suggestedMax":16}) m,
	VuoInputData(VuoReal, {"name":"n1", "default":2, "suggestedMin":0, "suggestedMax":10}) n1,
	VuoInputData(VuoReal, {"name":"n2", "default":5, "suggestedMin":0, "suggestedMax":10}) n2,
	VuoInputData(VuoReal, {"name":"n3", "default":2, "suggestedMin":0, "suggestedMax":10}) n3,
	VuoInputData(VuoReal, {"name":"a",  "default":1, "suggestedMin":0, "suggestedMax": 2}) a,
	VuoInputData(VuoReal, {"name":"b",  "default":1, "suggestedMin":0, "suggestedMax": 2}) b,
	VuoInputData(VuoRange, {"default":{"minimum":-180.0,"maximum":180.0},
							"requireMin":true,
							"requireMax":true,
							"suggestedMin":{"minimum":-360.0,"maximum":-360.0},
							"suggestedMax":{"minimum":360.0,"maximum":360.0},
							"suggestedStep":{"minimum":15.0,"maximum":15.0}}) angle,
	VuoInputData(VuoRange, {"default":{"minimum":1.0,"maximum":1.0},
							"requireMin":true,
							"requireMax":true,
							"suggestedMin":{"minimum":0.0,"maximum":0.0},
							"suggestedMax":{"minimum":1.0,"maximum":1.0},
							"suggestedStep":{"minimum":0.01,"maximum":0.01}}) radius,
	VuoInputData(VuoInteger, {"default": 64, "suggestedMin":4, "suggestedMax":256}) rows,
	VuoInputData(VuoInteger, {"default":128, "suggestedMin":4, "suggestedMax":256}) columns,
	VuoOutputData(VuoSceneObject) object
)
{
	// Prevent dividing by zero.
	double n1Clamped = MAX(0.0001, n1);
	double aClamped = MAX(0.0001, a);
	double bClamped = MAX(0.0001, b);

	unsigned int rowsClamped = MAX(4, MIN(512, rows));
	unsigned int columnsClamped = MAX(4, MIN(512, columns));

	// If the structure hasn't changed, just reuse the existing GPU mesh data.
	if (m == (*context)->m
	 && n1Clamped == (*context)->n1
	 && n2 == (*context)->n2
	 && n3 == (*context)->n3
	 && aClamped == (*context)->a
	 && bClamped == (*context)->b
	 && angle.minimum == (*context)->angle.minimum
	 && angle.maximum == (*context)->angle.maximum
	 && radius.minimum == (*context)->radius.minimum
	 && radius.maximum == (*context)->radius.maximum
	 && rowsClamped == (*context)->rows
	 && columnsClamped == (*context)->columns)
	{
		*object = VuoSceneObject_copy(*object);
		VuoSceneObject_setTransform(*object, transform);
		VuoSceneObject_setShader(*object, VuoShader_make_VuoGenericType1(material));
		return;
	}

	VuoDictionary_VuoText_VuoReal constants = VuoDictionaryCreate_VuoText_VuoReal();
	VuoDictionary_VuoText_VuoReal_retain(constants);
	VuoDictionarySetKeyValue_VuoText_VuoReal(constants, VuoText_make("m"), m);
	VuoDictionarySetKeyValue_VuoText_VuoReal(constants, VuoText_make("n1"), n1Clamped);
	VuoDictionarySetKeyValue_VuoText_VuoReal(constants, VuoText_make("n2"), n2);
	VuoDictionarySetKeyValue_VuoText_VuoReal(constants, VuoText_make("n3"), n3);
	VuoDictionarySetKeyValue_VuoText_VuoReal(constants, VuoText_make("a"), aClamped);
	VuoDictionarySetKeyValue_VuoText_VuoReal(constants, VuoText_make("b"), bClamped);
	VuoDictionarySetKeyValue_VuoText_VuoReal(constants, VuoText_make("uMin"), angle.minimum);
	VuoDictionarySetKeyValue_VuoText_VuoReal(constants, VuoText_make("uMax"), angle.maximum);
	VuoDictionarySetKeyValue_VuoText_VuoReal(constants, VuoText_make("r0"), radius.minimum);
	VuoDictionarySetKeyValue_VuoText_VuoReal(constants, VuoText_make("r1"), radius.maximum);

	bool closeU = VuoReal_areEqual(radius.minimum, radius.maximum)
		&& VuoReal_areEqual(fabs(angle.maximum - angle.minimum), 360);

	VuoMesh mesh = VuoMeshParametric_generate(
		0,
		"((abs(cos(m*u/4)/a)^n2 + abs(sin(m*u/4)/b)^n3) ^ (-1/n1)) * sin(u) * ((abs(cos(m*v/4)/a)^n2 + abs(sin(m*v/4)/b)^n3) ^ (-1/n1)) * cos(v) * mix(r0, r1, (u - uMin) / (uMax - uMin)) / 2.",
		"((abs(cos(m*v/4)/a)^n2 + abs(sin(m*v/4)/b)^n3) ^ (-1/n1)) * sin(v) / 2.",
		"((abs(cos(m*u/4)/a)^n2 + abs(sin(m*u/4)/b)^n3) ^ (-1/n1)) * cos(u) * ((abs(cos(m*v/4)/a)^n2 + abs(sin(m*v/4)/b)^n3) ^ (-1/n1)) * cos(v) * mix(r0, r1, (u - uMin) / (uMax - uMin)) / 2.",
		columnsClamped, rowsClamped,
		closeU,
		angle.minimum, angle.maximum,
		false,  // don't close v
		-90, 90,
		&constants);

	*object = VuoSceneObject_makeMesh(mesh, VuoShader_make_VuoGenericType1(material), transform);
	VuoDictionary_VuoText_VuoReal_release(constants);
	VuoSceneObject_setName(*object, VuoText_make("Supershape"));

	(*context)->m = m;
	(*context)->n1 = n1Clamped;
	(*context)->n2 = n2;
	(*context)->n3 = n3;
	(*context)->a = aClamped;
	(*context)->b = bClamped;
	(*context)->angle = angle;
	(*context)->radius = radius;
	(*context)->rows = rowsClamped;
	(*context)->columns = columnsClamped;
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) context
)
{
}
