/**
 * @file
 * vuo.logic.areAllFalse node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Are All False",
	"keywords": [
		"boolean", "condition", "test", "check", "gate", "and", "&&", "0", "1",
		"not true",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent(
	VuoInputData(VuoList_VuoBoolean) values,
	VuoOutputData(VuoBoolean) allFalse)
{
	*allFalse = true;
	unsigned long termsCount = VuoListGetCount_VuoBoolean(values);
	for (unsigned long i = 1; i <= termsCount; ++i)
		*allFalse = *allFalse && !VuoListGetValue_VuoBoolean(values, i);
}
