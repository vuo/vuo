/**
 * @file
 * vuo.math.multiply node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
    "title": "Multiply",
    "keywords": [
        "product", "times", "*", "•", "×", "x", "arithmetic", "calculate",
        "component-wise", "element-wise", "entrywise", "Hadamard product", "Schur product", "array", "vector",
    ],
    "version": "2.1.1",
    "genericTypes": {
        "VuoGenericType1": {
            "defaultType": "VuoReal",
            "compatibleTypes": [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ],
        },
    },
    "node": {
        "exampleCompositions": [ ],
    },
});

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) values,
		VuoOutputData(VuoGenericType1) product
)
{
	unsigned long termsCount = VuoListGetCount_VuoGenericType1(values);
	*product = 1;
	if (termsCount)
	{
		VuoGenericType1 *valuesData = VuoListGetData_VuoGenericType1(values);
		for (unsigned long i = 0; i < termsCount; ++i)
			*product *= valuesData[i];
	}
}
