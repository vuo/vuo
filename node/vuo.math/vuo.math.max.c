/**
 * @file
 * vuo.math.max node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Find Maximum",
					  "keywords" : [ "great", "large", "big", "high", "more", "most", ">", "top", "upper", "peak", "limit", "bound", "range" ],
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
		VuoOutputData(VuoGenericType1) max
)
{
	unsigned long termsCount = VuoListGetCount_VuoGenericType1(terms);

	VuoGenericType1 *termsArray = (VuoGenericType1 *)malloc(termsCount * sizeof(VuoGenericType1));
	for (unsigned long i = 0; i < termsCount; ++i)
		termsArray[i] = VuoListGetValueAtIndex_VuoGenericType1(terms, i+1);

	*max = VuoGenericType1_max(termsArray, termsCount);

	free(termsArray);
}
