/**
 * @file
 * vuo.dictionary.get.multiple.VuoText.VuoReal node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoDictionary_VuoText_VuoReal.h"

VuoModuleMetadata({
					  "title" : "Get Items from Dictionary",
					  "description": "Gets a list of keys and values from a dictionary.",
					  "keywords" : [ "pick", "select", "choose", "element", "member", "key", "index" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				 });


void nodeEvent
(
		VuoInputData(VuoDictionary_VuoText_VuoReal) dictionary,
		VuoInputData(VuoList_VuoText) keys,
		VuoOutputData(VuoList_VuoReal) values
)
{
	*values = VuoListCreate_VuoReal();

	unsigned long count = VuoListGetCount_VuoText(keys);
	for (unsigned long i = 1; i <= count; ++i)
	{
		VuoText key = VuoListGetValue_VuoText(keys, i);
		VuoReal value = VuoDictionaryGetValueForKey_VuoText_VuoReal(dictionary, key);
		VuoListAppendValue_VuoReal(*values, value);
	}
}
