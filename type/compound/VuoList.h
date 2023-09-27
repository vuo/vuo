/**
 * @file
 * VuoList interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoList_VuoGenericType1_DEFINED
#define VuoList_VuoGenericType1_DEFINED

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @ingroup VuoTypes
 * An ordered list of items, all of the same type.
 *
 * When accessing items in the list, the first item is at index 1 (not 0).
 * @{
 */

/**
 * A list of items.
 */
typedef const struct { void *l; } * VuoList_VuoGenericType1;

#ifdef VuoGenericType1_SUPPORTS_COMPARISON
#define VuoList_VuoGenericType1_SUPPORTS_COMPARISON  ///< Instances of this type can be compared and sorted.
#endif
#ifdef VuoGenericType1_OVERRIDES_INTERPROCESS_SERIALIZATION
#define VuoList_VuoGenericType1_OVERRIDES_INTERPROCESS_SERIALIZATION  ///< This type implements `_getInterprocessJson()`.
#endif

/**
 * Creates a new list of `VuoGenericType1` items.
 */
VuoList_VuoGenericType1 VuoListCreate_VuoGenericType1(void);

/**
 * Creates a new list of @a count instances of @a value.
 *
 * Use this in conjunction with @ref VuoListGetData_VuoGenericType1 to quickly initialize a large list.
 */
VuoList_VuoGenericType1 VuoListCreateWithCount_VuoGenericType1(const unsigned long count, const VuoGenericType1 value);

/**
 * Creates a new list containing the items in the C array @a values.
 *
 * The list retains each item in @a values. The caller may discard @a values after this function returns.
 */
VuoList_VuoGenericType1 VuoListCreateWithValueArray_VuoGenericType1(const VuoGenericType1 *values, const unsigned long valueCount);

/**
 * Makes a shallow copy of @a list — its items are retained (not copied) by the new list.
 */
VuoList_VuoGenericType1 VuoListCopy_VuoGenericType1(const VuoList_VuoGenericType1 list);

/**
 * Returns the item at @a index.
 *
 * Index values start at 1.
 *
 * If the list has no items, returns a default value (an empty/zero value of `VuoGenericType1`).
 *
 * Attempting to access an out-of-bounds index returns the first item in the list (if the index is 0),
 * or last item in the list (if the index is greater than the list size).
 *
 * If iterating over an entire list, consider using @ref VuoListGetData_VuoGenericType1 or @ref VuoListForeach_VuoGenericType1.
 */
VuoGenericType1 VuoListGetValue_VuoGenericType1(const VuoList_VuoGenericType1 list, const unsigned long index);

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
VuoGenericType1 * VuoListGetData_VuoGenericType1(const VuoList_VuoGenericType1 list);

/**
 * Applies @a function to each item in @a list, serially in order of index.
 *
 * If @a function returns false, visiting will stop immediately
 * (possibly before all items have been visited).
 *
 * @version200New
 */
void VuoListForeach_VuoGenericType1(const VuoList_VuoGenericType1 list, bool (^function)(const VuoGenericType1 value));

/**
 * Changes the item at @a index.
 *
 * Index values start at 1.
 *
 * If @a expandListIfNeeded is false — If the list has no items, nothing is changed.
 * Attempting to change an out-of-bounds index changes the first item in the list (if the index is 0), or last item in the list (if the index is greater than the list size).
 *
 * If @a expandListIfNeeded is true — If the list has less than @a index items, it is expanded to accommodate the specified index.
 * If @a index is 0, the list is resized to contain 1 item set to @a value.
 */
void VuoListSetValue_VuoGenericType1(const VuoList_VuoGenericType1 list, const VuoGenericType1 value, const unsigned long index, bool expandListIfNeeded);

/**
 * Inserts @a value immediately before @a index.

 * Index values start at 1.
 *
 * Inserting at index 0 prepends the value to the list.
 * Inserting at an index beyond the last value in the list appends the value to the list.
 */
void VuoListInsertValue_VuoGenericType1(const VuoList_VuoGenericType1 list, const VuoGenericType1 value, const unsigned long index);

/**
 * Prepends @a value to @a list.
 */
void VuoListPrependValue_VuoGenericType1(VuoList_VuoGenericType1 list, const VuoGenericType1 value);

/**
 * Appends @a value to @a list.
 */
void VuoListAppendValue_VuoGenericType1(VuoList_VuoGenericType1 list, const VuoGenericType1 value);

/**
 * Swaps the value at @a indexA with the value at @a indexB.
 */
void VuoListExchangeValues_VuoGenericType1(VuoList_VuoGenericType1 list, const unsigned long indexA, const unsigned long indexB);

#ifdef VuoGenericType1_SUPPORTS_COMPARISON
/**
 * Sorts @a list using the ordering defined by @ref VuoGenericType1_isLessThan.
 */
void VuoListSort_VuoGenericType1(VuoList_VuoGenericType1 list);

/**
 * Returns true if the two lists contain equivalent items (according to @ref VuoGenericType1_areEqual)
 * in the same order.
 *
 * NULL lists are never equal to non-NULL lists (even empty lists).
 */
bool VuoList_VuoGenericType1_areEqual(const VuoList_VuoGenericType1 a, const VuoList_VuoGenericType1 b);

/**
 * Returns true if @a a contains fewer items than @a b, or if @a a and @a b contain the same number of items
 * and the first item that differs between the two lists is lesser in @a a (according to @ref VuoGenericType1_isLessThan).
 */
bool VuoList_VuoGenericType1_isLessThan(const VuoList_VuoGenericType1 a, const VuoList_VuoGenericType1 b);
#endif

/**
 * Generates a random permutation of @a list.
 *
 * @a chaos ranges from 0 to 1.  When @a chaos is 1, a full Fisher–Yates shuffle is performed.  When less than 1, fewer iterations are performed.
 */
void VuoListShuffle_VuoGenericType1(VuoList_VuoGenericType1 list, const double chaos);

/**
 * Reverses the order of the items in @a list.
 */
void VuoListReverse_VuoGenericType1(VuoList_VuoGenericType1 list);

/**
 * Returns a new list containing items from the original list in the range specified by @a startIndex and @a itemCount.
 *
 * Items in the new list are retained (not copied) from the original list.
 */
VuoList_VuoGenericType1 VuoListSubset_VuoGenericType1(VuoList_VuoGenericType1 list, const signed long startIndex, const unsigned long itemCount);

#ifdef VuoGenericType1_SUPPORTS_COMPARISON
/**
 * Returns a new list containing unique items from the original list, preserving order.
 *
 * Items in the new list are retained (not copied) from the original list.
 *
 * @version200New
 */
VuoList_VuoGenericType1 VuoListRemoveDuplicates_VuoGenericType1(VuoList_VuoGenericType1 list);
#endif

/**
 * Removes the first item from @a list.
 */
void VuoListRemoveFirstValue_VuoGenericType1(VuoList_VuoGenericType1 list);

/**
 * Removes the last item from @a list.
 */
void VuoListRemoveLastValue_VuoGenericType1(VuoList_VuoGenericType1 list);

/**
 * Removes all items from @a list.
 */
void VuoListRemoveAll_VuoGenericType1(VuoList_VuoGenericType1 list);

/**
 * Removes the item at @a index.
 *
 * Index values start at 1.
 *
 * Attempting to remove index 0 or an index beyond the last value in the list has no effect.
 */
void VuoListRemoveValue_VuoGenericType1(VuoList_VuoGenericType1 list, const unsigned long index);

/**
 * Returns the number of items in @a list.
 */
unsigned long VuoListGetCount_VuoGenericType1(const VuoList_VuoGenericType1 list);

/**
 * Decodes the JSON object @a js to create a new list.
 *
 * @eg{
 *   ["uno", "dos", "tres", "catorce"]
 * }
 */
VuoList_VuoGenericType1 VuoList_VuoGenericType1_makeFromJson(struct json_object *js);

/**
 * Encodes @a list as a JSON object.
 */
struct json_object * VuoList_VuoGenericType1_getJson(const VuoList_VuoGenericType1 list);

#ifdef VuoGenericType1_OVERRIDES_INTERPROCESS_SERIALIZATION
/**
 * Encodes @a list as an interprocess-compatible JSON object.
 *
 * @version200New
 */
struct json_object * VuoList_VuoGenericType1_getInterprocessJson(const VuoList_VuoGenericType1 list);
#endif

/**
 * Produces a brief human-readable summary of @c list.
 */
char * VuoList_VuoGenericType1_getSummary(const VuoList_VuoGenericType1 list);

/**
 * Retains the list. This function is provided for use in polymorphism; it just calls `VuoRetain()`.
 */
void VuoList_VuoGenericType1_retain(VuoList_VuoGenericType1 list);

/**
 * Releases the list. This function is provided for use in polymorphism; it just calls `VuoRelease()`.
 */
void VuoList_VuoGenericType1_release(VuoList_VuoGenericType1 list);

/// @{
/**
 * Automatically generated function.
 */
char * VuoList_VuoGenericType1_getString(const VuoList_VuoGenericType1 list);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
