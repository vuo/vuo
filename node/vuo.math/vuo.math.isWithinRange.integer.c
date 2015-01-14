/**
 * @file
 * vuo.math.isWithinRange.integer node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Is within Range",
					 "keywords" : [ "clamp", "restrict", "wrap", "limit", "bound" ],
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
		VuoOutputData(VuoBoolean) withinRange
)
{
	*withinRange = (minimum <= value && value <= maximum);
}
