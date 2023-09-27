/**
 * @file
 * vuo.text.get.ascii node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <string.h>

VuoModuleMetadata({
	"title": "Get ASCII Code",
	"keywords": [
		"string",
		"symbol",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent
(
	VuoInputData(VuoText, {"default":"A"}) character,
	VuoOutputData(VuoInteger, {"name":"ASCII"}) ascii
)
{
	if (VuoText_isEmpty(character)
		|| character[1] != 0  // If text is more than 1 byte
		|| character[1] > 127 // If text is outside ASCII-7
	)
	{
		*ascii = 0;
		return;
	}

	*ascii = character[0];
}
