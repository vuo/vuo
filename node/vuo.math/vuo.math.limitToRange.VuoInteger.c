/**
 * @file
 * vuo.math.limitToRange.VuoInteger node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

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
		VuoInputData(VuoInteger, {"default": 0}) value,
		VuoInputData(VuoInteger, {"default": 0}) minimum,
		VuoInputData(VuoInteger, {"default": 10}) maximum,
		VuoInputData(VuoWrapMode, {"default":"wrap"}) wrapMode,
		VuoOutputData(VuoInteger) limitedValue
)
{
	*limitedValue = value;

	switch(wrapMode)
	{
		case VuoWrapMode_Wrap:
			if(*limitedValue > maximum)
				*limitedValue = minimum + ((*limitedValue-maximum-1) % (maximum-minimum+1));
			else if(*limitedValue < minimum)
				*limitedValue = maximum - ( (minimum-*limitedValue+1) % (maximum-minimum+1));
			break;

		case VuoWrapMode_Saturate:
			if(*limitedValue > maximum)
				*limitedValue = maximum;
			else if(*limitedValue < minimum)
				*limitedValue = minimum;

			break;
	}
}
