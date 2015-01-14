/**
 * @file
 * vuo.point.normalize.2d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Normalize 2D Point",
					  "description" :
						  "<p>Outputs a unit vector (length of 1) in the same direction as the input vector.</p>",
					  "keywords" : [ "unit", "vector", "length", "magnitude" ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : false
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}}) point,
		VuoOutputData(VuoPoint2d) normalizedPoint
)
{
	*normalizedPoint = VuoPoint2d_normalize(point);
}
