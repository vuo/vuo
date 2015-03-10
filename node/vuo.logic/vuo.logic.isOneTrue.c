/**
 * @file
 * vuo.logic.areSomeTrue node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Is One True",
					 "keywords" : [ "boolean", "condition", "test", "check", "gate", "xor", "^", "0", "1", "false" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoBoolean) terms,
		VuoOutputData(VuoBoolean) oneTrue
)
{
	int trueCount = 0;
	unsigned long termsCount = VuoListGetCount_VuoBoolean(terms);
	for (unsigned long i = 1; i <= termsCount; ++i)
		trueCount += VuoListGetValueAtIndex_VuoBoolean(terms, i);
	*oneTrue = (trueCount == 1);
}
