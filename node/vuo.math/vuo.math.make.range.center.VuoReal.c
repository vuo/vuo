/**
 * @file
 * vuo.math.make.range.center node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make Range (Real, Center/Radius)",
					  "keywords" : [ "spread", "diameter" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.5}) center,
		VuoInputData(VuoReal, {"default":0.1}) radius,
		VuoOutputData(VuoRange) range
)
{
	*range = VuoRange_make(center - radius, center + radius);
}
