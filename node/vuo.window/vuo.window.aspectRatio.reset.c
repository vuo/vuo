/**
 * @file
 * vuo.window.aspectRatio.reset node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoWindowProperty.h"

VuoModuleMetadata({
					 "title" : "Reset Window Aspect Ratio",
					 "keywords" : [ "width", "height", "dimensions", "lock", "fixed", "size", "change" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "ToggleAspectRatio.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoOutputData(VuoWindowProperty) property
)
{
	(*property).type = VuoWindowProperty_AspectRatioReset;
}
