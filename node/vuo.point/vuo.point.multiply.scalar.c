/**
 * @file
 * vuo.point.multiply.scalar node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Scale Point",
					  "keywords" : [ "multiply", "divide", "product", "*", "/", "vector" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoGenericType1, {"default":{"x":1,"y":1,"z":1,"w":1}}) point,
		VuoInputData(VuoReal, {"default":1.0}) scaleFactor,
		VuoOutputData(VuoGenericType1) scaledPoint
)
{
	*scaledPoint = VuoGenericType1_multiply(point, scaleFactor);
}
