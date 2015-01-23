/**
 * @file
 * vuo.math.min node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Find Minimum",
					  "keywords" : [ "less", "least", "small", "few", "low", "<", "bottom", "lower", "limit", "bound", "range" ],
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
		VuoInputData(VuoList_VuoGenericType1) terms,
		VuoOutputData(VuoGenericType1) min
)
{
	unsigned long termsCount = VuoListGetCount_VuoGenericType1(terms);

	VuoGenericType1 *termsArray = (VuoGenericType1 *)malloc(termsCount * sizeof(VuoGenericType1));
	for (unsigned long i = 0; i < termsCount; ++i)
		termsArray[i] = VuoListGetValueAtIndex_VuoGenericType1(terms, i+1);

	*min = VuoGenericType1_min(termsArray, termsCount);

	free(termsArray);
}
