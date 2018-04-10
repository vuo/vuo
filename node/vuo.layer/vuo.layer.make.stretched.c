/**
 * @file
 * vuo.layer.make.stretched node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Make Image Layer (Stretched)",
					 "keywords" : [ "billboard", "sprite", "stretch", "fill", "shrink", "blow up", "enlarge", "magnify" ],
					 "version" : "1.0.0",
					 "node": {}
				 });

void nodeEvent
(
		VuoInputData(VuoText) name,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoTransform2d) transform,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) opacity,
		VuoOutputData(VuoLayer) layer
)
{
	*layer = VuoLayer_makeWithTransform(name, image, transform, opacity);
}
