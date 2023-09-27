/**
 * @file
 * vuo.logic.areAnyFalse node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Are Any False",
	"keywords": [
		"boolean", "condition", "test", "check", "gate", "or", "||", "0", "1",
		"not true",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ],
	},
});

void nodeEvent(
	VuoInputData(VuoList_VuoBoolean) values,
	VuoOutputData(VuoBoolean) anyFalse)
{
	*anyFalse = false;
	unsigned long termsCount = VuoListGetCount_VuoBoolean(values);
	for (unsigned long i = 1; i <= termsCount; ++i)
		*anyFalse = *anyFalse || !VuoListGetValue_VuoBoolean(values, i);
}
