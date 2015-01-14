/**
 * @file
 * vuo.point.multiply.scalar.2d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Scale 2D Point",
					 "description" :
						"<p>Multiplies each coordinate of the point by a scale factor.</p>",
					 "keywords" : [ "multiply", "product", "*", "vector" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint2d, {"default":{"x":1,"y":1}}) point,
		VuoInputData(VuoReal, {"default":1}) scaleFactor,
		VuoOutputData(VuoPoint2d) scaledPoint
)
{
	*scaledPoint = VuoPoint2d_multiply(point, scaleFactor);
}
