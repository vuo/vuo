/**
 * @file
 * vuo.math.areEqual node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Are Equal",
					  "keywords" : [ "==", "same", "identical", "equivalent", "match", "compare", "approximate", "tolerance", "conditional" ],
					  "version" : "2.0.1",
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
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":0, "VuoReal":0.00001}}) tolerance,
		VuoOutputData(VuoBoolean) equal
)
{
	unsigned long termsCount = VuoListGetCount_VuoGenericType1(values);
	if (termsCount > 1)
	{
		VuoGenericType1 min, max;
		min = max = VuoListGetValue_VuoGenericType1(values, 1);
		for (unsigned long i = 2; i <= termsCount; ++i)
		{
			VuoGenericType1 term = VuoListGetValue_VuoGenericType1(values, i);
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
