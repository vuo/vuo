/**
 * @file
 * vuo.math.limitToRange.VuoReal node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <float.h>

VuoModuleMetadata({
					 "title" : "Limit to Range",
					 "keywords" : [ "clamp", "restrict", "wrap", "limit", "bound", "range" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default": 0.0}) value,
		VuoInputData(VuoReal, {"default": 0.0}) minimum,
		VuoInputData(VuoReal, {"default": 10.0}) maximum,
		VuoInputData(VuoWrapMode, {"default": "wrap"}) wrapMode,
		VuoOutputData(VuoReal) limitedValue
)
{
	*limitedValue = value;

	switch(wrapMode)
	{
		case VuoWrapMode_Wrap:
			if(*limitedValue > maximum)
				*limitedValue = minimum + fmod( *limitedValue-maximum, maximum-minimum);
			else if(*limitedValue < minimum)
				*limitedValue = maximum - fmod( minimum-*limitedValue, maximum-minimum);
			break;

		case VuoWrapMode_Saturate:
			if(*limitedValue > maximum)
				*limitedValue = maximum;
			else if(*limitedValue < minimum)
				*limitedValue = minimum;

			break;
	}
}
