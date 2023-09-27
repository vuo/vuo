/**
 * @file
 * vuo.point.distance node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Measure Distance",
					  "keywords" : [ "farness", "length", "hypotenuse" ],
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
		VuoInputData(VuoGenericType1, {"default":{"x":0,"y":0,"z":0,"w":0}}) a,
		VuoInputData(VuoGenericType1, {"default":{"x":0,"y":0,"z":0,"w":0}}) b,
		VuoOutputData(VuoReal) distance
)
{
	*distance = VuoGenericType1_distance(a, b);
}
