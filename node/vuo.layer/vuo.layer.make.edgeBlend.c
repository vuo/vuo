/**
 * @file
 * vuo.layer.make.edgeBlend node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"
#include "VuoEdgeBlend.h"

VuoModuleMetadata({
					 "title" : "Make Edge Blend",
					 "keywords" : [ "fade", "projection", "mapping" ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0, "suggestedMin":0, "suggestedMax":0.5, "suggestedStep":0.1}) crop,
		VuoInputData(VuoReal, {"default":0, "suggestedMin":0, "suggestedMax":0.5, "suggestedStep":0.1}) cutoff,
		VuoInputData(VuoReal, {"default":1, "suggestedMin":0.001, "suggestedMax":3, "suggestedStep":0.1}) gamma,
		VuoOutputData(VuoEdgeBlend) edgeBlend
)
{
	*edgeBlend = VuoEdgeBlend_make(cutoff, gamma, crop);
}
