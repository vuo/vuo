/**
 * @file
 * vuo.type.list.real.text node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Convert Real List to Text List",
	"description": "Outputs a list containing text representing the input list's real numbers.",
	"keywords": [
		"string",
	],
	"version": "1.0.0",
});

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) reals,
	VuoOutputData(VuoList_VuoText) texts
)
{
	unsigned long count = VuoListGetCount_VuoReal(reals);
	*texts = VuoListCreateWithCount_VuoText(count, 0);
	VuoReal *realsArray = VuoListGetData_VuoReal(reals);
	VuoText *textsArray = VuoListGetData_VuoText(*texts);
	for (unsigned long i = 0; i < count; ++i)
	{
		textsArray[i] = VuoText_format("%g", realsArray[i]);
		VuoRegister(textsArray[i], free);
		VuoRetain(textsArray[i]);
	}
}
