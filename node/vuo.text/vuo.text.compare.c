/**
 * @file
 * vuo.text.compare node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Compare Texts",
					 "keywords" : [
						"comparison", "same", "identical", "equivalent", "match", "approximate",
						"unequal", "inequality", "different",
						"contains", "begins with", "starts with", "ends with", "prefix", "suffix",
						"case", "sensitive", "insensitive"
					 ],
					 "version" : "1.0.0",
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
