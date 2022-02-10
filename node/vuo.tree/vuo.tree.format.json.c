/**
 * @file
 * vuo.tree.format.json node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTree.h"

VuoModuleMetadata({
	"title": "Format Tree as JSON",
	"keywords": [
		"text", "string",
		"convert", "serialize", "export", "save", "write",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ "RecordMouseClicksToJsonFile.vuo" ],
	},
});

void nodeEvent
(
		VuoInputData(VuoTree) tree,
		VuoInputData(VuoBoolean, {"default":true}) indent,
		VuoOutputData(VuoText, {"name":"JSON"}) json
)
{
	*json = VuoTree_serializeAsJson(tree, indent);
}
