/**
 * @file
 * vuo.point.make.parametric.grid node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "stdlib.h"
#include "stdio.h"
#include "VuoPointsParametric.h"

VuoModuleMetadata({
					 "title" : "Make Parametric Grid Points",
					 "keywords" : [ "math", "expression", "shape" ],
					 "version" : "1.0.1",
					 "dependencies" : [
						 "VuoPointsParametric"
					 ],
					  "node": {
						  "exampleCompositions" : [ "MakeSmoothTerrain.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) time,
		VuoInputData(VuoText, {"default":"u"}) xExpression,
		VuoInputData(VuoText, {"default":"v"}) yExpression,
		VuoInputData(VuoText, {"default":"0"}) zExpression,
		VuoInputData(VuoInteger, {"default":16,"suggestedMin":1}) rows,
		VuoInputData(VuoInteger, {"default":16,"suggestedMin":1}) columns,
		VuoInputData(VuoReal, {"default":0.0, "suggestedStep":0.1}) uMin,
		VuoInputData(VuoReal, {"default":1.0, "suggestedStep":0.1}) uMax,
		VuoInputData(VuoReal, {"default":0.0, "suggestedStep":0.1}) vMin,
		VuoInputData(VuoReal, {"default":1.0, "suggestedStep":0.1}) vMax,
		VuoOutputData(VuoList_VuoPoint3d) points
)
{
	*points = VuoPointsParametric2d_generate(
		time,
		xExpression,
		yExpression,
		zExpression,
		rows,
		columns,
		uMin,
		uMax,
		vMin,
		vMax);
}
