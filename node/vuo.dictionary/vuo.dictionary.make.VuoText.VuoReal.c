/**
 * @file
 * vuo.dictionary.make.VuoText.VuoReal node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoDictionary_VuoText_VuoReal.h"

VuoModuleMetadata({
					  "title" : "Make Dictionary",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				 });


void nodeEvent
(
		VuoInputData(VuoList_VuoText) keys,
		VuoInputData(VuoList_VuoReal) values,
		VuoOutputData(VuoDictionary_VuoText_VuoReal) dictionary
)
{
	*dictionary = VuoDictionaryCreate_VuoText_VuoReal();

	unsigned long count = MIN( VuoListGetCount_VuoText(keys), VuoListGetCount_VuoReal(values) );
	for (unsigned long i = 1; i <= count; ++i)
	{
		VuoText key = VuoListGetValue_VuoText(keys, i);
		VuoReal value = VuoListGetValue_VuoReal(values, i);
		VuoDictionarySetKeyValue_VuoText_VuoReal(*dictionary, key, value);
	}
}
