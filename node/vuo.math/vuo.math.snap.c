/**
 * @file
 * vuo.math.snap node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Snap Value",
					 "keywords" : [  "quantize", "grid", "approximate", "preset", "discrete", "integer", "nearby", "close" ],
					 "version" : "1.0.0",
					 "genericTypes" : {
						 "VuoGenericType1" : {
							// "defaultType" : "VuoReal",
							"compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ]
						 }
					 },
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoGenericType1) value,
		VuoInputData(VuoGenericType1) spacing,
		VuoInputData(VuoGenericType1) center,
		VuoOutputData(VuoGenericType1) snappedValue
)
{
	*snappedValue = VuoGenericType1_snap(value, center, spacing);
}
