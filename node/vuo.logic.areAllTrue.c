/**
 * @file
 * vuo.logic.areAllTrue node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Are All True",
					 "description" :
						"<p>Outputs <i>true</i> if all terms are <i>true</i>.</p> \
						<p>If there are no terms, outputs <i>true</i>.</p> \
						<p>This node is useful for making something happen only if several conditions are met. \
						You can connect this node's output port to the `which` input port of a `Select Input` or `Select Output` node.</p>",
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
