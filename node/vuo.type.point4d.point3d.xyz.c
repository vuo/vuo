/**
 * @file
 * vuo.type.point4d.point3d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert 4D Point to 3D Point",
					 "description" :
						 "<p>Outputs just the (x,y,z) part of a 4D point (x,y,z,w).</p> \
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
		VuoOutputData(VuoPoint3d) xyz
)
{
	*xyz = VuoPoint3d_make(xyzw.x, xyzw.y, xyzw.z);
}
