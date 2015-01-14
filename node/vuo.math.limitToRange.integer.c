/**
 * @file
 * vuo.math.limitToRange.real node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
// #include <int.h>

VuoModuleMetadata({
					 "title" : "Limit to Range",
					 "description" :
						"<p>Restricts passed value to min and max using provided wrap mode.</p>",
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
		VuoInputData(VuoCountWrapMode, {"default":"wrap"}) wrapMode,
		VuoOutputData(VuoInteger) limitedValue
)
{
	*limitedValue = value;

	switch(wrapMode)
	{
		case VuoCountWrapMode_Wrap:
			if(*limitedValue > maximum)
				*limitedValue = minimum + ((*limitedValue-maximum) % (maximum-minimum));

			if(*limitedValue < minimum)
				*limitedValue = maximum - ( (minimum - *limitedValue) % (maximum-minimum));
			break;

		case VuoCountWrapMode_Saturate:
			if(*limitedValue > maximum)
				*limitedValue = maximum;
			else if(*limitedValue < minimum)
				*limitedValue = minimum;

			break;
	}
}
