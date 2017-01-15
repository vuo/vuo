/**
 * @file
 * vuo.type.real.point2d node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Real to 2D Point (X,0)",
					 "keywords" : [ ],
					 "version" : "1.0.1"
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) x,
		VuoOutputData(VuoPoint2d, {"name":"(X,0)"}) point2d
)
{
	*point2d = VuoPoint2d_make(x, 0.);
}
