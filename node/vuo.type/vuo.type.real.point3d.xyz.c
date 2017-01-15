/**
 * @file
 * vuo.type.real.point3d.xyz node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Real to 3D Point (X,X,X)",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) x,
		VuoOutputData(VuoPoint3d, {"name":"(X,X,X)"}) point3d
)
{
	*point3d = VuoPoint3d_make(x, x, x);
}
