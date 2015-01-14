/**
 * @file
 * vuo.type.point4d.point2d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert 4D Point to 2D Point",
					 "description" :
						 "<p>Outputs just the (x,y) part of a 4D point (x,y,z,w).</p> \
						 <p>This node is useful as a type converter to connect ports with different data types.</p>",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint4d, {"default":{"x":0, "y":0, "z":0, "w":0}}) xyzw,
		VuoOutputData(VuoPoint2d) xy
)
{
	*xy = VuoPoint2d_make(xyzw.x, xyzw.y);
}
