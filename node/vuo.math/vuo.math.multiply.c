/**
 * @file
 * vuo.math.multiply node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
    "title": "Multiply",
    "keywords": [
        "product", "times", "*", "•", "×", "x", "arithmetic", "calculate",
        "component-wise", "element-wise", "entrywise", "Hadamard product", "Schur product", "array", "vector",
    ],
    "version": "2.1.0",
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
		for (unsigned long i = 1; i <= termsCount; ++i)
			*product *= VuoListGetValue_VuoGenericType1(values, i);
}
