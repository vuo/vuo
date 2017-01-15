/**
 * @file
 * vuo.text.split.stream node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Split Text Stream",
					  "keywords" : [ "tokenize", "explode", "part", "piece", "word", "line", "delimiter", "csv",
									 "substring", "string" ],
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

VuoText * nodeInstanceInit(void)
{
	VuoText *t = (VuoText *)malloc(sizeof(VuoText));
	VuoRegister(t, free);
	*t = VuoText_make("");
	VuoRetain(*t);
	return t;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoText *) accumulatedText,
		VuoInputData(VuoText) text,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoText":" "}}) separator,
		VuoInputData(VuoBoolean, {"default":false}) includeEmptyParts,
		VuoOutputTrigger(splitText, VuoText)
)
{
	VuoText combinedTextArray[] = { **accumulatedText, text };
	VuoText combinedText = VuoText_append(combinedTextArray, 2);
	size_t combinedTextLength = VuoText_length(combinedText);
	size_t separatorLength = VuoText_length(separator);

	VuoText splittableText;
	VuoRelease(**accumulatedText);
	if (separatorLength > 0)
	{
		size_t lastSeparatorIndex = VuoText_findLastOccurrence(combinedText, separator);
		if (lastSeparatorIndex == 0)
			lastSeparatorIndex = 1 - separatorLength;
		splittableText = VuoText_substring(combinedText, 1, lastSeparatorIndex - 1);
		**accumulatedText = VuoText_substring(combinedText, lastSeparatorIndex + separatorLength, combinedTextLength);
	}
	else
	{
		splittableText = combinedText;
		**accumulatedText = VuoText_make("");
	}
	VuoRetain(**accumulatedText);

	size_t partsCount = 0;
	VuoText *splitTextsArr = VuoText_split(splittableText, separator, includeEmptyParts, &partsCount);

	for (size_t i = 0; i < partsCount; ++i)
		splitText( splitTextsArr[i] );

	if (separatorLength > 0)
	{
		VuoRetain(splittableText);
		VuoRelease(splittableText);
	}
	VuoRetain(combinedText);
	VuoRelease(combinedText);
	free(splitTextsArr);
}

void nodeInstanceFini
(
		VuoInstanceData(VuoText *) accumulatedText
)
{
	VuoRelease(**accumulatedText);
}
