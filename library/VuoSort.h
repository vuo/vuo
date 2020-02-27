/**
 * @file
 * VuoSort interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Array item type for `VuoSort_sortArrayByOtherArray()`.
 */
typedef struct
{
	int index;  ///< The item's index before sorting.
	float value;  ///< The item's value to compare to other items when sorting.
} VuoIndexedFloat;

void VuoSort_sortArrayByOtherArray(void *array, unsigned long elemCount, unsigned long elemSize, VuoIndexedFloat *other);

#ifdef __cplusplus
}
#endif
