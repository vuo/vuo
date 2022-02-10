/**
 * @file
 * LIST_TYPE C type definition.
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
 * @defgroup LIST_TYPE LIST_TYPE
 * A list of @ref ELEMENT_TYPE elements.
 *
 * @{
 */

#error "This header is a template; do not include it."

#if defined(DOXYGEN) || !defined(LIST_TYPE_TYPE_DEFINED)
/// @{
/**
 * A list of @ref ELEMENT_TYPE elements.
 */
typedef const struct LIST_TYPE_struct { void *l; } * LIST_TYPE;
/// @}
#endif

/**
 * Creates a new list of @ref ELEMENT_TYPE elements.
 */
LIST_TYPE VuoListCreate_ELEMENT_TYPE(void);

/**
 * Creates a new list of `count` instances of `value`.
 *
 * Use this in conjunction with @ref VuoListGetData_ELEMENT_TYPE to quickly initialize a large list.
 */
LIST_TYPE VuoListCreateWithCount_ELEMENT_TYPE(const unsigned long count, const ELEMENT_TYPE value);

/**
 * Creates a new list containing the items in the C array `values`.
 *
 * The list retains each item in `values`.
 *
 * The caller may discard `values` after this function returns.
 */
LIST_TYPE VuoListCreateWithValueArray_ELEMENT_TYPE(const ELEMENT_TYPE *values, const unsigned long valueCount);

/**
 * Makes a shallow copy of `list` — its items are retained (not copied) by the new list.
 */
LIST_TYPE VuoListCopy_ELEMENT_TYPE(const LIST_TYPE list);

#ifndef VUO_LIST_INDEX_TO_C_ARRAY_INDEX
#define VUO_LIST_INDEX_TO_C_ARRAY_INDEX
/**
 * Converts a Vuo-style `[1..n]` list index into a C-style `[0..n-1]` list index.
 *
 * If the index is less than 1, or larger than the size of the list, the returned index is clamped.
 */
static inline unsigned long VuoListIndexToCArrayIndex(long index, unsigned long listCount) __attribute__((const));
static inline unsigned long VuoListIndexToCArrayIndex(long index, unsigned long listCount)
{
    if (index <= 0)
        return 0;

    if (index > listCount)
        return listCount - 1;

    return index - 1;
}
#endif

/**
 * Returns the @ref ELEMENT_TYPE at @c index.
 * Index values start at 1.
 * If the list has no items, returns a default value.
 * Attempting to access an out-of-bounds index returns the first item in the list (if the index is 0), or last item in the list (if the index is greater than the list size).
 *
 * If iterating over an entire list, consider using @ref VuoListGetData_ELEMENT_TYPE or @ref VuoListForeach_ELEMENT_TYPE.
 */
ELEMENT_TYPE VuoListGetValue_ELEMENT_TYPE(const LIST_TYPE list, const unsigned long index);

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
ELEMENT_TYPE *VuoListGetData_ELEMENT_TYPE(const LIST_TYPE list);

/**
 * Applies `function` to each of `list`'s items, serially in order of index.
 *
 * If `function` returns false, visiting will stop immediately
 * (possibly before all items have been visited).
 *
 * @version200New
 */
void VuoListForeach_ELEMENT_TYPE(const LIST_TYPE list, bool (^function)(const ELEMENT_TYPE value));

/**
 * Changes the @ref ELEMENT_TYPE at @c index.
 * Index values start at 1.
 *
 * If `expandListIfNeeded` is false: If the list has no items, nothing is changed.
 * Attempting to change an out-of-bounds index changes the first item in the list (if the index is 0), or last item in the list (if the index is greater than the list size).
 *
 * If `expandListIfNeeded` is true: If the list has less than `index` elements, it is expanded to accommodate the specified `index`.
 * If `index` is 0, the list is resized to contain 1 item set to `value`.
 */
void VuoListSetValue_ELEMENT_TYPE(const LIST_TYPE list, const ELEMENT_TYPE value, const unsigned long index, bool expandListIfNeeded);

/**
 * Inserts the `ELEMENT_TYPE` immediately before `index`.
 * Index values start at 1.
 * Inserting at index 0 prepends the value to the list.
 * Inserting at an index beyond the last value in the list appends the value to the list.
 */
void VuoListInsertValue_ELEMENT_TYPE(const LIST_TYPE list, const ELEMENT_TYPE value, const unsigned long index);

/**
 * Prepends @c value to @c list.
 */
void VuoListPrependValue_ELEMENT_TYPE(LIST_TYPE list, const ELEMENT_TYPE value);

/**
 * Appends @c value to @c list.
 */
void VuoListAppendValue_ELEMENT_TYPE(LIST_TYPE list, const ELEMENT_TYPE value);

/**
 * Swaps the value at `indexA` with the value at `indexB`.
 */
void VuoListExchangeValues_ELEMENT_TYPE(LIST_TYPE list, const unsigned long indexA, const unsigned long indexB);

#ifdef ELEMENT_TYPE_SUPPORTS_COMPARISON
/**
 * Sorts `list`.
 */
void VuoListSort_ELEMENT_TYPE(LIST_TYPE list);

/**
 * Returns true if the two lists are equivalent.
 *
 * NULL lists are never equal to non-NULL lists (even empty lists).
 */
bool LIST_TYPE_areEqual(const LIST_TYPE a, const LIST_TYPE b);

/**
 * Returns true if `a` < `b`.
 */
bool LIST_TYPE_isLessThan(const LIST_TYPE a, const LIST_TYPE b);
#endif

/**
 * Generates a random permutation of `list`.
 *
 * `chaos` ranges from 0 to 1.  When `chaos` is 1, a full Fisher–Yates shuffle is performed.  When less than 1, fewer iterations are performed.
 */
void VuoListShuffle_ELEMENT_TYPE(LIST_TYPE list, const double chaos);

/**
 * Reverses the order of the items in `list`.
 */
void VuoListReverse_ELEMENT_TYPE(LIST_TYPE list);

/**
 * Returns a new list containing items from the original list in the range specified by `startIndex` and `itemCount`.
 *
 * Items in the new list are retained (not copied) from the original list.
 */
LIST_TYPE VuoListSubset_ELEMENT_TYPE(LIST_TYPE list, const signed long startIndex, const unsigned long itemCount);

#ifdef ELEMENT_TYPE_SUPPORTS_COMPARISON
/**
 * Returns a new list containing unique items from the original list, preserving order.
 *
 * Items in the new list are retained (not copied) from the original list.
 *
 * @version200New
 */
LIST_TYPE VuoListRemoveDuplicates_ELEMENT_TYPE(LIST_TYPE list);
#endif

/**
 * Removes the first value from @c list.
 */
void VuoListRemoveFirstValue_ELEMENT_TYPE(LIST_TYPE list);

/**
 * Removes the last value from @c list.
 */
void VuoListRemoveLastValue_ELEMENT_TYPE(LIST_TYPE list);

/**
 * Removes all values from @c list.
 */
void VuoListRemoveAll_ELEMENT_TYPE(LIST_TYPE list);

/**
 * Removes the `ELEMENT_TYPE` at `index`.
 * Index values start at 1.
 * Attempting to remove index 0 or an index beyond the last value in the list has no effect.
 */
void VuoListRemoveValue_ELEMENT_TYPE(LIST_TYPE list, const unsigned long index);

/**
 * Returns the number of elements in @c list.
 */
unsigned long VuoListGetCount_ELEMENT_TYPE(const LIST_TYPE list);

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *	["uno", "dos", "tres", "catorce"]
 * }
 */
LIST_TYPE LIST_TYPE_makeFromJson(struct json_object * js);

/**
 * Encodes @c value as a JSON object.
 */
struct json_object * LIST_TYPE_getJson(const LIST_TYPE value);

#ifdef ELEMENT_TYPE_REQUIRES_INTERPROCESS_JSON
/**
 * Encodes `value` as an interprocess-compatible JSON object.
 *
 * @version200New
 */
struct json_object * LIST_TYPE_getInterprocessJson(const LIST_TYPE value);
#endif

/**
 * Produces a brief human-readable summary of @c value.
 */
char * LIST_TYPE_getSummary(const LIST_TYPE value);

/**
 * Retains the list. This function is provided for use in polymorphism; it just calls `VuoRetain()`.
 */
void LIST_TYPE_retain(LIST_TYPE value);

/**
 * Releases the list. This function is provided for use in polymorphism; it just calls `VuoRelease()`.
 */
void LIST_TYPE_release(LIST_TYPE value);

/// @{
/**
 * Automatically generated function.
 */
char * LIST_TYPE_getString(const LIST_TYPE value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
