/**
 * @file
 * vuo.text.split node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Split Text",
					  "keywords" : [ "tokenize", "explode", "part", "piece", "word", "line", "delimiter", "csv",
									 "substring", "string",
									 /* QC */ "String Components"
					  ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							 "defaultType" : "VuoText",
							 "compatibleTypes" : [ "VuoText" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [  ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoText) text,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoText":" "}}) separator,
		VuoInputData(VuoBoolean, {"default":false}) includeEmptyParts,
		VuoOutputData(VuoList_VuoText) splitTexts
)
{
	size_t partsCount = 0;
	VuoText *splitTextsArr = VuoText_split(text, separator, includeEmptyParts, &partsCount);

	*splitTexts = VuoListCreate_VuoText();
	for (size_t i = 0; i < partsCount; ++i)
		VuoListAppendValue_VuoText(*splitTexts, splitTextsArr[i]);

	free(splitTextsArr);
}
