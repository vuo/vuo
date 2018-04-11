/**
 * @file
 * vuo.dictionary.get.multiple.VuoText.VuoText node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoDictionary_VuoText_VuoText.h"

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
		VuoInputData(VuoDictionary_VuoText_VuoText) dictionary,
		VuoInputData(VuoList_VuoText) keys,
		VuoOutputData(VuoList_VuoText) values
)
{
	*values = VuoListCreate_VuoText();

	unsigned long count = VuoListGetCount_VuoText(keys);
	for (unsigned long i = 1; i <= count; ++i)
	{
		VuoText key = VuoListGetValue_VuoText(keys, i);
		VuoText value = VuoDictionaryGetValueForKey_VuoText_VuoText(dictionary, key);
		VuoListAppendValue_VuoText(*values, value);
	}
}
