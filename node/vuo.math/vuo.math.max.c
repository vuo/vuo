/**
 * @file
 * vuo.math.max node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Find Maximum",
					  "keywords" : [ "greatest", "large", "big", "high", "more", "most", ">", "top", "upper", "peak", "limit", "bound", "range" ],
					  "version" : "2.0.0",
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
		VuoOutputData(VuoGenericType1) max
)
{
	unsigned long termsCount = VuoListGetCount_VuoGenericType1(values);

	VuoGenericType1 *termsArray = (VuoGenericType1 *)malloc(termsCount * sizeof(VuoGenericType1));
	for (unsigned long i = 0; i < termsCount; ++i)
		termsArray[i] = VuoListGetValue_VuoGenericType1(values, i+1);

	*max = VuoGenericType1_max(termsArray, termsCount);

	free(termsArray);
}
