/**
 * @file
 * vuo.math.make.range node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make Range (Real, Min/Max)",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":true}) hasMinimum,
		VuoInputData(VuoReal, {"default":1}) minimum,
		VuoInputData(VuoBoolean, {"default":true}) hasMaximum,
		VuoInputData(VuoReal, {"default":10}) maximum,
		VuoOutputData(VuoRange) range
)
{
	*range = VuoRange_make(hasMinimum ? minimum : VuoRange_NoMinimum,
						   hasMaximum ? maximum : VuoRange_NoMaximum);
}
