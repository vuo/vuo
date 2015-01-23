/**
 * @file
 * vuo.logic.areAllTrue node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Are All True",
					 "keywords" : [ "boolean", "condition", "test", "check", "gate", "and", "&" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoBoolean) terms,
		VuoOutputData(VuoBoolean) allTrue
)
{
	*allTrue = true;
	unsigned long termsCount = VuoListGetCount_VuoBoolean(terms);
	for (unsigned long i = 1; i <= termsCount; ++i)
		*allTrue = *allTrue && VuoListGetValueAtIndex_VuoBoolean(terms, i);
}
