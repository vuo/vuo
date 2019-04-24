/**
 * @file
 * vuo.math.min node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Find Minimum",
					  "keywords" : [ "less", "least", "small", "few", "low", "<", "bottom", "lower", "limit", "bound", "range" ],
					  "version" : "2.1.0",
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

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) values,
		VuoOutputData(VuoGenericType1, {"name":"Minimum"}) min,
		VuoOutputData(VuoInteger) position
)
{
	unsigned long termsCount = VuoListGetCount_VuoGenericType1(values);
	VuoGenericType1 *termsArray = VuoListGetData_VuoGenericType1(values);
	*min = VuoGenericType1_min(termsArray, termsCount, position);
}
