/**
 * @file
 * vuo.math.get.range node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Get Range Values (Integer)",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoIntegerRange) range,
		VuoOutputData(VuoBoolean) hasMinimum,
		VuoOutputData(VuoInteger) minimum,
		VuoOutputData(VuoBoolean) hasMaximum,
		VuoOutputData(VuoInteger) maximum
)
{
	*hasMinimum = range.minimum != VuoIntegerRange_NoMinimum;
	*minimum = range.minimum;
	*hasMaximum = range.maximum != VuoIntegerRange_NoMaximum;
	*maximum = range.maximum;
}
