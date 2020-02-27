/**
 * @file
 * vuo.image.areEqual node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Are Equal (Image)",
					  "keywords" : [ "==", "same", "identical", "equivalent", "match", "compare", "approximate", "tolerance", "conditional" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoImage) images,
		VuoInputData(VuoReal, {"default":0.01,"suggestedMin":0.0,"suggestedMax":1.0,"suggestedStep":0.1}) tolerance,
		VuoOutputData(VuoBoolean) equal
)
{
	unsigned long termsCount = VuoListGetCount_VuoImage(images);
	if (termsCount > 1)
	{
		// Compare every image with every other image.
		// E.g., for 3 images, compare 1-2, 1-3, and 2-3.
		for (unsigned long i = 1; i < termsCount; ++i)
		{
			VuoImage a = VuoListGetValue_VuoImage(images, i);
			for (unsigned long j = i+1; j <= termsCount; ++j)
			{
				VuoImage b = VuoListGetValue_VuoImage(images, j);
				if (!VuoImage_areEqualWithinTolerance(a, b, tolerance * 255))
				{
					*equal = false;
					return;
				}
			}
		}

		*equal = true;
	}
	else
	{
		*equal = true;
	}
}
