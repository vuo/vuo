/**
 * @file
 * vuo.type.data.text node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoData.h"

VuoModuleMetadata({
	"title": "Convert Data to Text",
	"keywords": [
		"string",
	],
	"version": "1.0.0",
});

void nodeEvent
(
		VuoInputData(VuoData) data,
		VuoOutputData(VuoText) text
)
{
	*text = VuoText_makeFromData((unsigned char *)data.data, data.size);
}
