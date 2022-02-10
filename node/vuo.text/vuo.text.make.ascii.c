/**
 * @file
 * vuo.text.make.ascii node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include <string.h>

VuoModuleMetadata({
	"title": "Make Text from ASCII",
	"keywords": [
		"string",
		"utf", "unicode", "symbol",
		"carriage", "return", "newline",
		"line", "tab", "em", "en",
		"space", "break", "invisible",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions" : [ "ShowAsciiCharacters.vuo" ],
	},
});

void nodeEvent
(
	VuoInputData(VuoInteger, {"default":65, "suggestedMin":0, "suggestedMax":127, "suggestedStep":1, "name":"ASCII"}) ascii,
	VuoOutputData(VuoText) text
)
{
	int val = labs(ascii);
	val = (val < 127 ? val : 127);

	*text = VuoText_make( (char*) &val );
}
