/**
 * @file
 * vuo.type.text.boolean node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include <stdlib.h>

VuoModuleMetadata({
	"title": "Convert Text to Boolean",
	"keywords": [
		"string",
		"0", "1", "true", "false",
	],
	"version": "1.0.0",
});

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) text,
		VuoOutputData(VuoBoolean) boolean
)
{
	if (!text)
		return;

	*boolean = VuoMakeRetainedFromString(text, VuoBoolean);
}
