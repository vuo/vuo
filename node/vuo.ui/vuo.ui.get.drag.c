/**
 * @file
 * vuo.ui.get.drag node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoDragEvent.h"

VuoModuleMetadata({
					  "title" : "Get Drag Values",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "ShowDraggedImages.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoDragEvent) drag,
		VuoOutputData(VuoPoint2d) position,
		VuoOutputData(VuoList_VuoText, {"name":"URLs"}) urls
)
{
	*position = drag.position;
	*urls = (VuoList_VuoText)drag.urls;
}
