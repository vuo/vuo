/**
 * @file
 * vuo.point.within.rectangle node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Is within Rectangle",
					 "keywords" : [ "contains", "bounds", "limit", "test", "distance", "box" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "IsMouseWithinRectangle.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}}) point,
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}}) center,
		VuoInputData(VuoReal, {"default":1.0}) width,
		VuoInputData(VuoReal, {"default":1.0}) height, VuoOutputData(VuoBoolean) withinRectangle
)
{
	*withinRectangle =
			center.x - width/2. <= point.x && point.x <= center.x + width/2. &&
			center.y - height/2. <= point.y && point.y <= center.y + height/2.;
}
