/**
 * @file
 * vuo.vertices.make.parametric node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "stdlib.h"
#include "stdio.h"
#include "VuoVerticesParametric.h"

VuoModuleMetadata({
					 "title" : "Make Parametric Vertices",
					 "keywords" : [ "math", "shape" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoVerticesParametric"
					 ],
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"u"}) xExpression,
		VuoInputData(VuoText, {"default":"v"}) yExpression,
		VuoInputData(VuoText, {"default":"0"}) zExpression,
		VuoInputData(VuoText, {"default":"u"}) uExpression,
		VuoInputData(VuoText, {"default":"v"}) vExpression,
		VuoInputData(VuoInteger, {"default":16,"suggestedMin":0}) rows,
		VuoInputData(VuoInteger, {"default":16,"suggestedMin":0}) columns,
		VuoInputData(VuoBoolean, {"default":false}) uClosed,
		VuoInputData(VuoBoolean, {"default":false}) vClosed,
		VuoOutputData(VuoList_VuoVertices) vertices
)
{
	*vertices = VuoVerticesParametric_generate( xExpression, yExpression, zExpression, uExpression, vExpression, rows, columns, (bool)uClosed, (bool)vClosed );
}
