/**
 * @file
 * vuo.math.add node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Add",
					 "keywords" : [ "sum", "plus", "total", "+", "arithmetic", "calculate" ],
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
		VuoOutputData(VuoGenericType1) sum
)
{
	*sum = 0;
	unsigned long termsCount = VuoListGetCount_VuoGenericType1(terms);
	for (unsigned long i = 1; i <= termsCount; ++i)
		*sum += VuoListGetValueAtIndex_VuoGenericType1(terms, i);
}
