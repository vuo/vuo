/**
 * @file
 * vuo.layer.populated node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					  "title" : "Is Layer Populated",
					  "keywords" : [ "backdrop", "background", "billboard", "non-empty", "nonempty" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoLayer) layer,
		VuoOutputData(VuoBoolean) populated
)
{
	*populated = VuoLayer_isPopulated(layer);
}
