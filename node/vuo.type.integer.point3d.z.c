/**
 * @file
 * vuo.type.integer.point3d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Integer to 3D Point",
					 "description" :
						 "<p>Creates a 3D point with coordinates (0,0,z).</p> \
						 <p>This node is useful as a type converter to connect ports with different data types.</p>",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0}) z,
		VuoOutputData(VuoPoint3d) point3d
)
{
	*point3d = VuoPoint3d_make(0, 0, z);
}
