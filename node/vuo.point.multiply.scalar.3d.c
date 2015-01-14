/**
 * @file
 * vuo.point.multiply.scalar.3d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Scale 3D Point",
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
		VuoInputData(VuoPoint3d, {"default":{"x":1,"y":1,"z":1}}) point,
		VuoInputData(VuoReal, {"default":1}) scaleFactor,
		VuoOutputData(VuoPoint3d) scaledPoint
)
{
	*scaledPoint = VuoPoint3d_multiply(point, scaleFactor);
}
