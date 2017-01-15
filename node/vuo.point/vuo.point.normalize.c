/**
 * @file
 * vuo.point.normalize node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Normalize Point",
					  "keywords" : [ "unit", "vector", "length", "magnitude" ],
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
		VuoOutputData(VuoGenericType1) normalizedPoint
)
{
	*normalizedPoint = VuoGenericType1_normalize(point);
}
