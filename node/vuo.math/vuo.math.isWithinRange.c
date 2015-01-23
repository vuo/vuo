/**
 * @file
 * vuo.math.isWithinRange node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Is within Range",
					  "keywords" : [ "clamp", "restrict", "wrap", "limit", "bound" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes" : [ "VuoInteger", "VuoReal" ]
						  }
					  },
					  "node": {
						  "isInterface" : false
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoGenericType1, {"default": 0}) value,
		VuoInputData(VuoGenericType1, {"default": 0}) minimum,
		VuoInputData(VuoGenericType1, {"default": 10}) maximum,
		VuoOutputData(VuoBoolean) withinRange
)
{
	*withinRange = (minimum <= value && value <= maximum);
}
