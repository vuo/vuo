/**
 * @file
 * vuo.math.snap node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Snap Value",
					 "keywords" : [  "quantize", "grid", "approximate", "preset", "discrete", "integer", "nearby", "close", "round" ],
					 "version" : "1.0.0",
					 "genericTypes" : {
						 "VuoGenericType1" : {
							// "defaultType" : "VuoReal",
							"compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ]
						 }
					 },
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":0,  "VuoReal":0.0, "VuoPoint2d":{"x":0.0,"y":0.0}, "VuoPoint3d":{"x":0.0,"y":0.0,"z":0.0}, "VuoPoint4d":{"x":0.0,"y":0.0,"z":0.0,"w":0.0}}}) value,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":10, "VuoReal":0.1, "VuoPoint2d":{"x":0.1,"y":0.1}, "VuoPoint3d":{"x":0.1,"y":0.1,"z":0.1}, "VuoPoint4d":{"x":0.1,"y":0.1,"z":0.1,"w":0.1}}}) spacing,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":0,  "VuoReal":0.0, "VuoPoint2d":{"x":0.0,"y":0.0}, "VuoPoint3d":{"x":0.0,"y":0.0,"z":0.0}, "VuoPoint4d":{"x":0.0,"y":0.0,"z":0.0,"w":0.0}}}) center,
		VuoOutputData(VuoGenericType1) snappedValue
)
{
	*snappedValue = VuoGenericType1_snap(value, center, spacing);
}
