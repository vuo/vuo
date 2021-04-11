/**
 * @file
 * vuo.math.absolute node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Calculate Absolute Value",
					  "keywords" : [ "sign", "positive", "|" ],
					  "version" : "1.0.1",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes" : [ "VuoInteger", "VuoReal" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

static VuoReal VuoReal_Absolute(VuoReal val)
{
	return fabs(val);
}

static VuoInteger VuoInteger_Absolute(VuoInteger val)
{
	return llabs(val);
}

#pragma clang diagnostic pop

void nodeEvent
(
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":0, "VuoReal":0.0}}) signedValue,
		VuoOutputData(VuoGenericType1) absoluteValue
)
{
	*absoluteValue = VuoGenericType1_Absolute(signedValue);
}
