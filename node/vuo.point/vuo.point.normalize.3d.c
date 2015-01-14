/**
 * @file
 * vuo.point.normalize.3d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Normalize 3D Point",
					  "keywords" : [ "unit", "vector", "length", "magnitude" ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : false
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0}}) point,
		VuoOutputData(VuoPoint3d) normalizedPoint
)
{
	*normalizedPoint = VuoPoint3d_normalize(point);
}
