/**
 * @file
 * vuo.math.areEqual node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Are Equal",
					  "keywords" : [ "==", "same", "identical", "equivalent", "match", "compare", "approximate", "tolerance", "conditional" ],
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
		VuoInputData(VuoGenericType1, {"default":0.01}) tolerance,
		VuoOutputData(VuoBoolean) equal
)
{
	unsigned long termsCount = VuoListGetCount_VuoGenericType1(terms);
	if (termsCount > 1)
	{
		VuoGenericType1 min, max;
		min = max = VuoListGetValueAtIndex_VuoGenericType1(terms, 1);
		for (unsigned long i = 2; i <= termsCount; ++i)
		{
			VuoGenericType1 term = VuoListGetValueAtIndex_VuoGenericType1(terms, i);
			if (term < min)
				min = term;
			if (term > max)
				max = term;
		}
		*equal = (max - min <= tolerance);
	}
	else
	{
		*equal = true;
	}
}
