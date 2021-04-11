/**
 * @file
 * vuo.type.anchor.horizontal node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Convert Anchor to Horizontal Alignment",
	"description": "Outputs the anchor's horizontal component.",
	"keywords": [ ],
	"version": "1.0.0",
});

void nodeEvent
(
	VuoInputData(VuoAnchor) anchor,
	VuoOutputData(VuoHorizontalAlignment) horizontal
)
{
	*horizontal = VuoAnchor_getHorizontal(anchor);
}
