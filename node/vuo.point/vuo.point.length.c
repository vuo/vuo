/**
 * @file
 * vuo.point.length node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Measure Length",
					  "keywords" : [ "farness", "distance", "hypotenuse", "vector" ],
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
		VuoInputData(VuoGenericType1, {"default":{"x":0,"y":0,"z":0,"w":0}}) point,
		VuoOutputData(VuoReal) length
)
{
	*length = VuoGenericType1_magnitude(point);
}
