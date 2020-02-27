/**
 * @file
 * vuo.text.compare node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Compare Texts",
					 "keywords" : [
						"comparison", "same", "identical", "equivalent", "match", "approximate",
						"unequal", "inequality", "different",
						"contains", "begins with", "starts with", "ends with", "prefix", "suffix",
						"case", "sensitive", "insensitive",
						"glob", "wildcard",
						"grep", "regex", "regular expression",
					 ],
					 "version" : "1.1.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) a,
		VuoInputData(VuoTextComparison, {"default":{"type":"equals","isCaseSensitive":false}}) comparison,
		VuoInputData(VuoText, {"default":""}) b,
		VuoOutputData(VuoBoolean) result
)
{
	*result = VuoText_compare(a, comparison, b);
}
