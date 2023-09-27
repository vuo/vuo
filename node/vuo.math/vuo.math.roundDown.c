/**
 * @file
 * vuo.math.roundDown node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "math.h"

VuoModuleMetadata({
					 "title" : "Round Down",
					 "keywords" : [ "floor", "near", "close", "approximate", "integer", "whole", "real" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) real,
		VuoOutputData(VuoInteger) roundedDown
)
{
	*roundedDown = (VuoInteger)floor(real);
}
