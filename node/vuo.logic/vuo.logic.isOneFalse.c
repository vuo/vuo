/**
 * @file
 * vuo.logic.isOneFalse node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Is One False",
	"keywords": [
		"boolean", "condition", "test", "check", "gate", "xor", "^", "0", "1",
		"not true",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent(
	VuoInputData(VuoList_VuoBoolean) values,
	VuoOutputData(VuoBoolean) oneFalse)
{
	int falseCount = 0;
	unsigned long termsCount = VuoListGetCount_VuoBoolean(values);
	for (unsigned long i = 1; i <= termsCount; ++i)
		falseCount += !VuoListGetValue_VuoBoolean(values, i);
	*oneFalse = (falseCount == 1);
}
