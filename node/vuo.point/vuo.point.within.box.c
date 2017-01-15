/**
 * @file
 * vuo.point.within.box node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Is within Box",
					 "keywords" : [ "contains", "bounds", "limit", "test", "distance", "cube" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0}}) point,
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0}}) center,
		VuoInputData(VuoReal, {"default":1.0}) width,
		VuoInputData(VuoReal, {"default":1.0}) height,
		VuoInputData(VuoReal, {"default":1.0}) depth,
		VuoOutputData(VuoBoolean) withinBox
)
{
	*withinBox =
			center.x - width/2. <= point.x && point.x <= center.x + width/2. &&
			center.y - height/2. <= point.y && point.y <= center.y + height/2. &&
			center.z - depth/2. <= point.z && point.z <= center.z + depth/2.;
}
