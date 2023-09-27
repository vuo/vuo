/**
 * @file
 * vuo.window.aspectRatio node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoWindowProperty.h"

VuoModuleMetadata({
					 "title" : "Lock Window Aspect Ratio",
					 "keywords" : [ "width", "height", "dimensions", "lock", "fixed", "size", "properties", "set" ],
					 "version" : "1.0.1",
					 "node": {
						 "isDeprecated": true,
						 "exampleCompositions" : [ "ToggleAspectRatio.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":4.}) width,
		VuoInputData(VuoReal, {"default":3.}) height,
		VuoOutputData(VuoWindowProperty) property
)
{
	(*property).type = VuoWindowProperty_AspectRatio;
	(*property).aspectRatio = width/height;
}
