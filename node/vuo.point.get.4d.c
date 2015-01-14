/**
 * @file
 * vuo.point.get.4d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get 4D Point Values",
					 "description" :
						"<p>Gives the coordinates of a 4D point.</p>",
					 "keywords" : [ "xyzw", "homogenous", "coordinates" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint4d, {"default":{"x":0,"y":0,"z":0,"w":0}}) point,
		VuoOutputData(VuoReal) x,
		VuoOutputData(VuoReal) y,
		VuoOutputData(VuoReal) z,
		VuoOutputData(VuoReal) w
)
{
	*x = point.x;
	*y = point.y;
	*z = point.z;
	*w = point.w;
}
