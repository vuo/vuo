/**
 * @file
 * VuoList_ExampleLanguage C type definition.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoList_ExampleLanguage VuoList_ExampleLanguage
 * A list of @ref ExampleLanguage elements.
 *
 * @{
 */

// This header is generated by vuo/type/list/generateVariants.sh.

#if defined(DOXYGEN) || !defined(VuoList_ExampleLanguage_TYPE_DEFINED)
/// @{
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
 * Creates a new list of `count` instances of `value`.
 *
 * Use this in conjunction with @ref VuoListGetData_ExampleLanguage to quickly initialize a large list.
 */
VuoList_ExampleLanguage VuoListCreateWithCount_ExampleLanguage(const unsigned long count, const ExampleLanguage value);

/**
 * Makes a shallow copy of `list` — its items are retained (not copied) by the new list.
 */
VuoList_ExampleLanguage VuoListCopy_ExampleLanguage(const VuoList_ExampleLanguage list);

/**
 * Returns the @ref ExampleLanguage at @c index.
 * Index values start at 1.
 * If the list has no items, returns a default value.
 * Attempting to access an out-of-bounds index returns the first item in the list (if the index is 0), or last item in the list (if the index is greater than the list size).
 *
 * If iterating over an entire list, consider using @ref VuoListGetData_ExampleLanguage or @ref VuoListForeach_ExampleLanguage.
 */
ExampleLanguage VuoListGetValue_ExampleLanguage(const VuoList_ExampleLanguage list, const unsigned long index);

/**
 * Returns a pointer to a C array containing the list items.
 *
 * Use this if you need a fast way to get or change multiple list items.
 *
 * You can modify values in the list by changing them in this array.
 * Just don't attempt to access beyond the list size.
 *
 * The pointer becomes invalid if you modify the list size using other functions
 * (e.g., Insert, Prepend, Append, Cut, Remove);
 * if you use those functions, just get a new pointer by calling this function again.
 *
 * If the list has no items, returns NULL.
 *
 * The pointer remains owned by the list; don't free it.
 */
ExampleLanguage *VuoListGetData_ExampleLanguage(const VuoList_ExampleLanguage list);

/**
 * Applies `function` to each of `list`'s items, serially in order of index.
 *
 * If `function` returns false, visiting will stop immediately
 * (possibly before all items have been visited).
 *
 * @version200New
 */
void VuoListForeach_ExampleLanguage(const VuoList_ExampleLanguage list, bool (^function)(const ExampleLanguage value));

/**
 * Changes the @ref ExampleLanguage at @c index.
 * Index values start at 1.
 *
 * If `expandListIfNeeded` is false: If the list has no items, nothing is changed.
 * Attempting to change an out-of-bounds index changes the first item in the list (if the index is 0), or last item in the list (if the index is greater than the list size).
 *
 * If `expandListIfNeeded` is true: If the list has less than `index` elements, it is expanded to accommodate the specified `index`.
 * If `index` is 0, the list is resized to contain 1 item set to `value`.
 */
void VuoListSetValue_ExampleLanguage(const VuoList_ExampleLanguage list, const ExampleLanguage value, const unsigned long index, bool expandListIfNeeded);

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

#ifdef ExampleLanguage_SUPPORTS_COMPARISON
/**
 * Sorts `list`.
 */
void VuoListSort_ExampleLanguage(VuoList_ExampleLanguage list);

/**
 * Returns true if the two lists are equivalent.
 *
 * NULL lists are never equal to non-NULL lists (even empty lists).
 */
bool VuoList_ExampleLanguage_areEqual(const VuoList_ExampleLanguage a, const VuoList_ExampleLanguage b);

/**
 * Returns true if `a` < `b`.
 */
bool VuoList_ExampleLanguage_isLessThan(const VuoList_ExampleLanguage a, const VuoList_ExampleLanguage b);
#endif

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
 * Returns a new list containing items from the original list in the range specified by `startIndex` and `itemCount`.
 *
 * Items in the new list are retained (not copied) from the original list.
 */
VuoList_ExampleLanguage VuoListSubset_ExampleLanguage(VuoList_ExampleLanguage list, const signed long startIndex, const unsigned long itemCount);

#ifdef ExampleLanguage_SUPPORTS_COMPARISON
/**
 * Returns a new list containing unique items from the original list, preserving order.
 *
 * Items in the new list are retained (not copied) from the original list.
 *
 * @version200New
 */
VuoList_ExampleLanguage VuoListRemoveDuplicates_ExampleLanguage(VuoList_ExampleLanguage list);
#endif

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

#ifdef ExampleLanguage_REQUIRES_INTERPROCESS_JSON
/**
 * Encodes `value` as an interprocess-compatible JSON object.
 *
 * @version200New
 */
struct json_object * VuoList_ExampleLanguage_getInterprocessJson(const VuoList_ExampleLanguage value);
#endif

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

#ifdef __cplusplus
}
#endif
