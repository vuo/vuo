/**
 * @file
 * vuo.math.scale node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include <float.h>

VuoModuleMetadata({
					  "title" : "Scale",
					  "keywords" : [ "convert", "unit", "range", "split" ],
					  "version" : "2.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes" : [ "VuoReal", "VuoPoint2d", "VuoPoint3d" ]
						  }
					  },
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoGenericType1, {"defaults":{"VuoReal":0., "VuoPoint2d":{"x":0.,"y":0.}, "VuoPoint3d":{"x":0.,"y":0.,"z":0.}}}) value,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoReal":0., "VuoPoint2d":{"x":0.,"y":0.}, "VuoPoint3d":{"x":0.,"y":0.,"z":0.}}}) start,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoReal":1., "VuoPoint2d":{"x":1.,"y":1.}, "VuoPoint3d":{"x":1.,"y":1.,"z":1.}}}) end,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoReal":0., "VuoPoint2d":{"x":0.,"y":0.}, "VuoPoint3d":{"x":0.,"y":0.,"z":0.}}}) scaledStart,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoReal":100., "VuoPoint2d":{"x":100.,"y":100.}, "VuoPoint3d":{"x":100.,"y":100.,"z":100.}}}) scaledEnd,
		VuoOutputData(VuoGenericType1) scaledValue
)
{
	VuoGenericType1 range = VuoGenericType1_subtract(end, start);
	range = VuoGenericType1_makeNonzero(range);
	VuoGenericType1 scaledRange = VuoGenericType1_subtract(scaledEnd, scaledStart);
	*scaledValue = VuoGenericType1_add( VuoGenericType1_divide( VuoGenericType1_scale( VuoGenericType1_subtract(value, start), scaledRange), range), scaledStart);
}
