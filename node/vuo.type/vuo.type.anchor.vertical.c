/**
 * @file
 * vuo.type.anchor.vertical node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Convert Anchor to Vertical Alignment",
	"description": "Outputs the anchor's vertical component.",
	"keywords": [ ],
	"version": "1.0.0",
});

void nodeEvent
(
	VuoInputData(VuoAnchor) anchor,
	VuoOutputData(VuoVerticalAlignment) vertical
)
{
	*vertical = VuoAnchor_getVertical(anchor);
}
