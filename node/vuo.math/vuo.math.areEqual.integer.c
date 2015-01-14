/**
 * @file
 * vuo.math.areEqual.integer node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Are Equal",
					 "keywords" : [ "same", "identical", "equivalent", "match", "compare", "conditional" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoInteger) terms,
		VuoOutputData(VuoBoolean) equal
)
{
	*equal = true;
	unsigned long termsCount = VuoListGetCount_VuoInteger(terms);
	for (unsigned long i = 2; i <= termsCount && *equal; ++i)
		if (VuoListGetValueAtIndex_VuoInteger(terms, i-1) != VuoListGetValueAtIndex_VuoInteger(terms, i))
			*equal = false;
}
