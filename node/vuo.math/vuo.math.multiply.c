/**
 * @file
 * vuo.math.multiply node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Multiply",
					  "keywords" : [ "product", "times", "*", "arithmetic", "calculate" ],
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
		VuoOutputData(VuoGenericType1) product
)
{
	unsigned long termsCount = VuoListGetCount_VuoGenericType1(values);
	*product = 1;
	if (termsCount)
		for (unsigned long i = 1; i <= termsCount; ++i)
			*product *= VuoListGetValue_VuoGenericType1(values, i);
}
