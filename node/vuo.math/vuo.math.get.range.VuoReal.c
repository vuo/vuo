/**
 * @file
 * vuo.math.get.range node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Get Range Values (Real)",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoRange) range,
		VuoOutputData(VuoBoolean) hasMinimum,
		VuoOutputData(VuoReal) minimum,
		VuoOutputData(VuoBoolean) hasMaximum,
		VuoOutputData(VuoReal) maximum
)
{
	*hasMinimum = range.minimum != VuoRange_NoMinimum;
	*minimum = range.minimum;
	*hasMaximum = range.maximum != VuoRange_NoMaximum;
	*maximum = range.maximum;
}
