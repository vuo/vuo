/**
 * @file
 * VuoList_ExampleLanguage interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VuoList_ExampleLanguage_H
#define VuoList_ExampleLanguage_H

#if !defined(VuoList_ExampleLanguage_TYPE_DEFINED)
/**
 * A list of @ref ExampleLanguage elements.
 */
typedef const struct VuoList_ExampleLanguage_struct { void *l; } * VuoList_ExampleLanguage;
/// @}
#endif

/**
 * Creates a new list of @ref ExampleLanguage elements.
 */
VuoList_ExampleLanguage VuoListCreate_ExampleLanguage(void);

/**
 * Makes a shallow copy of `list` — its items are retained (not copied) by the new list.
 */
VuoList_ExampleLanguage VuoListCopy_ExampleLanguage(const VuoList_ExampleLanguage list);

/**
 * Returns the @ref ExampleLanguage at @c index.
 * Index values start at 1.
 * If the list has no items, returns a default value.
 * Attempting to access an out-of-bounds index returns the first item in the list (if the index is 0), or last item in the list (if the index is greater than the list size).
 */
ExampleLanguage VuoListGetValue_ExampleLanguage(const VuoList_ExampleLanguage list, const unsigned long index);

/**
 * Changes the @ref ExampleLanguage at @c index.
 * Index values start at 1.
 * If the list has no items, nothing is changed.
 * Attempting to change an out-of-bounds index changes the first item in the list (if the index is 0), or last item in the list (if the index is greater than the list size).
 */
void VuoListSetValue_ExampleLanguage(const VuoList_ExampleLanguage list, const ExampleLanguage value, const unsigned long index);

/**
 * Inserts the `ExampleLanguage` immediately before `index`.
 * Index values start at 1.
 * Inserting at index 0 prepends the value to the list.
 * Inserting at an index beyond the last value in the list appends the value to the list.
 */
void VuoListInsertValue_ExampleLanguage(const VuoList_ExampleLanguage list, const ExampleLanguage value, const unsigned long index);

/**
 * Prepends @c value to @c list.
 */
void VuoListPrependValue_ExampleLanguage(VuoList_ExampleLanguage list, const ExampleLanguage value);

/**
 * Appends @c value to @c list.
 */
void VuoListAppendValue_ExampleLanguage(VuoList_ExampleLanguage list, const ExampleLanguage value);

/**
 * Swaps the value at `indexA` with the value at `indexB`.
 */
void VuoListExchangeValues_ExampleLanguage(VuoList_ExampleLanguage list, const unsigned long indexA, const unsigned long indexB);

/**
 * Generates a random permutation of `list`.
 *
 * `chaos` ranges from 0 to 1.  When `chaos` is 1, a full Fisher–Yates shuffle is performed.  When less than 1, fewer iterations are performed.
 */
void VuoListShuffle_ExampleLanguage(VuoList_ExampleLanguage list, const double chaos);

/**
 * Reverses the order of the items in `list`.
 */
void VuoListReverse_ExampleLanguage(VuoList_ExampleLanguage list);

/**
 * Removes items from the list except those in the range specified by `startIndex` and `itemCount`.
 */
void VuoListCut_ExampleLanguage(VuoList_ExampleLanguage list, const signed long startIndex, const unsigned long itemCount);

/**
 * Removes the first value from @c list.
 */
void VuoListRemoveFirstValue_ExampleLanguage(VuoList_ExampleLanguage list);

/**
 * Removes the last value from @c list.
 */
void VuoListRemoveLastValue_ExampleLanguage(VuoList_ExampleLanguage list);

/**
 * Removes all values from @c list.
 */
void VuoListRemoveAll_ExampleLanguage(VuoList_ExampleLanguage list);

/**
 * Removes the `ExampleLanguage` at `index`.
 * Index values start at 1.
 * Attempting to remove index 0 or an index beyond the last value in the list has no effect.
 */
void VuoListRemoveValue_ExampleLanguage(VuoList_ExampleLanguage list, const unsigned long index);

/**
 * Returns the number of elements in @c list.
 */
unsigned long VuoListGetCount_ExampleLanguage(const VuoList_ExampleLanguage list);

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *	["uno", "dos", "tres", "catorce"]
 * }
 */
VuoList_ExampleLanguage VuoList_ExampleLanguage_makeFromJson(struct json_object * js);

/**
 * Encodes @c value as a JSON object.
 */
struct json_object * VuoList_ExampleLanguage_getJson(const VuoList_ExampleLanguage value);

/**
 * Produces a brief human-readable summary of @c value.
 */
char * VuoList_ExampleLanguage_getSummary(const VuoList_ExampleLanguage value);

/// @{
/**
 * Automatically generated function.
 */
VuoList_ExampleLanguage VuoList_ExampleLanguage_makeFromString(const char *str);
char * VuoList_ExampleLanguage_getString(const VuoList_ExampleLanguage value);
/// @}

/**
 * @}
 */

#endif
