/**
 * @file
 * vuo.point.distance node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Measure Distance between Points",
					  "keywords" : [ "farness", "length", "hypotenuse" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ]
						  }
					  },
					  "node": {
						  "isInterface" : false
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoGenericType1, {"default":{"x":0,"y":0,"z":0,"w":0}}) a,
		VuoInputData(VuoGenericType1, {"default":{"x":0,"y":0,"z":0,"w":0}}) b,
		VuoOutputData(VuoReal) distance
)
{
	*distance = VuoGenericType1_distance(a, b);
}
