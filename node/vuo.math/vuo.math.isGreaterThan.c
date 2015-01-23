/**
 * @file
 * vuo.math.isGreaterThan node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Is Greater Than",
					  "keywords" : [ "large", "big", "high", "more", "most", ">", "compare", "conditional" ],
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
		VuoInputData(VuoGenericType1, {"default":0.0}) a,
		VuoInputData(VuoGenericType1, {"default":0.0}) b,
		VuoOutputData(VuoBoolean) greaterThan
)
{
	*greaterThan = a > b;
}
