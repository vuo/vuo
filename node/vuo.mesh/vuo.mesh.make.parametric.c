/**
 * @file
 * vuo.vertices.make.parametric node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "stdlib.h"
#include "stdio.h"
#include "VuoMeshParametric.h"

VuoModuleMetadata({
					 "title" : "Make Parametric Grid Mesh",
					 "keywords" : [ "math", "expression", "shape", "surface", "plane", "grid" ],
					 "version" : "3.0.0",
					 "dependencies" : [
						 "VuoMeshParametric"
					 ],
					 "node": {
						 "exampleCompositions" : [ "SpinShell.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) time,
		VuoInputData(VuoText, {"default":"u"}) xExpression,
		VuoInputData(VuoText, {"default":"v"}) yExpression,
		VuoInputData(VuoText, {"default":"0"}) zExpression,
		VuoInputData(VuoInteger, {"default":16,"suggestedMin":2}) rows,
		VuoInputData(VuoInteger, {"default":16,"suggestedMin":2}) columns,
		VuoInputData(VuoBoolean, {"default":false}) uClosed,
		VuoInputData(VuoReal, {"default":0.0, "suggestedStep":0.1}) uMin,
		VuoInputData(VuoReal, {"default":1.0, "suggestedStep":0.1}) uMax,
		VuoInputData(VuoBoolean, {"default":false}) vClosed,
		VuoInputData(VuoReal, {"default":0.0, "suggestedStep":0.1}) vMin,
		VuoInputData(VuoReal, {"default":1.0, "suggestedStep":0.1}) vMax,
		VuoOutputData(VuoMesh) mesh
)
{
	*mesh = VuoMeshParametric_generate(time, xExpression, yExpression, zExpression, columns, rows, (bool)uClosed, uMin, uMax, (bool)vClosed, vMin, vMax);
}
