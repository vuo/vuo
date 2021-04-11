/**
 * @file
 * VuoSort implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSort.h"

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoSort",
					  "dependencies" : [
					  ]
				 });
#endif

/**
 * Helper function for `VuoSort_sortArrayByOtherArray()`. Returns a negative, zero, or positive result
 * depending on whether the value of @a a is less then, equal to, or greater than the value of @a b.
 */
static int compareFloats(const void *a, const void *b)
{
	VuoIndexedFloat *aa = (VuoIndexedFloat *)a;
	VuoIndexedFloat *bb = (VuoIndexedFloat *)b;

	return copysign(1, aa->value - bb->value);
}

/**
 * Puts both @a array and @a other in ascending order of @a other's elements' values.
 */
void VuoSort_sortArrayByOtherArray(void *array, unsigned long elemCount, unsigned long elemSize, VuoIndexedFloat *other)
{
	qsort(other, elemCount, sizeof(VuoIndexedFloat), compareFloats);

	char *srcBytes = (char *)malloc(elemCount * elemSize);
	memcpy(srcBytes, array, elemCount * elemSize);

	char *dstBytes = (char *)array;

	for (size_t i = 0; i < elemCount; ++i)
		memcpy(dstBytes + i * elemSize, srcBytes + other[i].index * elemSize, elemSize);

	free(srcBytes);
}
