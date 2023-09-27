/**
 * @file
 * vuo.window.aspectRatio.reset node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoWindowProperty.h"

VuoModuleMetadata({
					 "title" : "Unlock Window Aspect Ratio",
					 "keywords" : [ "width", "height", "dimensions", "lock", "fixed", "size", "change", "properties", "set" ],
					 "version" : "1.1.0",
					 "node": {
						 "isDeprecated": true,
						 "exampleCompositions" : [ "ToggleAspectRatio.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputEvent() unlock,
		VuoOutputData(VuoWindowProperty) property
)
{
	// This conditional is commented out to enable compositions
	// using the deprecated refresh port to continue working.
//	if (unlock)
	(*property).type = VuoWindowProperty_AspectRatioReset;
}
