﻿/**
 * @file
 * vuo.math.sin node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Calculate Sine",
					  "keywords" : [ "trigonometry", "angle", "triangle" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoReal) angle,
		VuoOutputData(VuoReal) sine
)
{
	double radians = angle * M_PI / 180.;
	*sine = sin(radians);
}
