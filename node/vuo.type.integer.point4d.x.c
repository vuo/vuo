/**
 * @file
 * vuo.type.integer.point4d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Integer to 4D Point",
					 "description" :
						 "<p>Creates a 4D point with coordinates (x,0,0,0).</p> \
						 <p>This node is useful as a type converter to connect ports with different data types.</p>",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0}) x,
		VuoOutputData(VuoPoint4d) point4d
)
{
	*point4d = VuoPoint4d_make(x, 0, 0, 0);
}
