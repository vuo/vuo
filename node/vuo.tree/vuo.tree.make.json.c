/**
 * @file
 * vuo.tree.make.json node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTree.h"

VuoModuleMetadata({
	"title": "Make Tree from JSON",
	"keywords": [
		"hierarchy", "hierarchical", "structure", "parent", "descendant", "leaf",
		"xml", "json", "html", "dom", "element", "tag", "object",
		"parse", "convert", "read",
		"string",
	],
	"version": "1.0.1",
	"node": {
		"exampleCompositions": [ "ListCountries.vuo", "LookUpCountry.vuo", "LookUpSisterCities.vuo" ],
	},
});

void nodeEvent
(
		VuoInputData(VuoText, {"name":"Text"}) json,
		VuoOutputData(VuoTree) tree
)
{
	*tree = VuoTree_makeFromJsonText(json);
}
