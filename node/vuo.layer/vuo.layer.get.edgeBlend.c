/**
 * @file
 * vuo.layer.get.edgeBlend node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"
#include "VuoEdgeBlend.h"

VuoModuleMetadata({
					 "title" : "Get Edge Blend Values",
					 "keywords" : [ "fade", "projection", "mapping" ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoEdgeBlend,
			{
				"default": {"cutoff":0, "gamma":1, "crop":0},
				"suggestedMin": {"cutoff":0, "gamma":0.001, "crop":0},
				"suggestedMax": {"cutoff":0.5, "gamma":3, "crop":0.5},
				"suggestedStep": {"cutoff":0.1, "gamma":0.1, "crop":0.1}
			}) edgeBlend,
		VuoOutputData(VuoReal) crop,
		VuoOutputData(VuoReal) cutoff,
		VuoOutputData(VuoReal) gamma
)
{
	*cutoff = edgeBlend.cutoff;
	*gamma = edgeBlend.gamma;
	*crop = edgeBlend.crop;
}
