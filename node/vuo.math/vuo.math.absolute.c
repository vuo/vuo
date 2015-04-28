/**
 * @file
 * vuo.math.absolute node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Calculate Absolute Value",
					  "keywords" : [ "sign", "positive" ],
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

VuoReal VuoReal_Absolute(VuoReal val)
{
	return fabs(val);
}

VuoInteger VuoInteger_Absolute(VuoInteger val)
{
	return abs(val);
}

void nodeEvent
(
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":0, "VuoReal":0.0}}) signedValue,
		VuoOutputData(VuoGenericType1) absoluteValue
)
{
	*absoluteValue = VuoGenericType1_Absolute(signedValue);
}
