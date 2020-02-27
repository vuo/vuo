/**
 * @file
 * vuo.math.tan node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Calculate Tangent",
					  "keywords" : [ "trigonometry", "angle", "triangle" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoReal) angle,
		VuoOutputData(VuoReal) tangent
)
{
	double radians = angle * M_PI / 180.;
	*tangent = tan(radians);
}
