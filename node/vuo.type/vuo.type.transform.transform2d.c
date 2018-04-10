/**
 * @file
 * vuo.type.transform.transform3d node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert 3D Transform to 2D",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoTransform, {"name":"3D Transform"}) transform,
		VuoOutputData(VuoTransform2d, {"name":"2D Transform"}) transform2d
)
{
	*transform2d = VuoTransform_get2d(transform);
}
