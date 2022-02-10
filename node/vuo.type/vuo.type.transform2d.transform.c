/**
 * @file
 * vuo.type.transform2d.transform node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert 2D Transform to 3D",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoTransform2d, {"name":"2D Transform"}) transform2d,
		VuoOutputData(VuoTransform, {"name":"3D Transform"}) transform
)
{
	*transform = VuoTransform_makeFrom2d(transform2d);
}
