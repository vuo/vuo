/**
 * @file
 * LIST_TYPE C type definition.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef LIST_TYPE_H
#define LIST_TYPE_H

/**
 * @ingroup VuoTypes
 * @defgroup LIST_TYPE LIST_TYPE
 * A list of @ref ELEMENT_TYPE elements.
 *
 * @{
 */

#error "This header is a template; do not include it."

/**
 * @typedef LIST_TYPE
 * A list of @ref ELEMENT_TYPE elements.
 */
#ifndef LIST_TYPE_TYPE_DEFINED
typedef void * LIST_TYPE;
#endif

/**
 * Creates a new list of @ref ELEMENT_TYPE elements.
 */
LIST_TYPE VuoListCreate_ELEMENT_TYPE(void);

/**
 * Returns the @ref ELEMENT_TYPE at @c index.
 * Index values start at 1.
 * If the list has no items, returns a default value.
 * Attempting to access an out-of-bounds index returns the first item in the list (if the index is 0), or last item in the list (if the index is greater than the list size).
 */
ELEMENT_TYPE VuoListGetValueAtIndex_ELEMENT_TYPE(const LIST_TYPE list, const unsigned long index);

/**
 * Appends @c value to @c list.
 */
void VuoListAppendValue_ELEMENT_TYPE(LIST_TYPE list, const ELEMENT_TYPE value);

/**
 * Removes the last value from @c list.
 */
void VuoListRemoveLastValue_ELEMENT_TYPE(LIST_TYPE list);

/**
 * Removes all values from @c list.
 */
void VuoListRemoveAll_ELEMENT_TYPE(LIST_TYPE list);

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
LIST_TYPE LIST_TYPE_valueFromJson(struct json_object * js);

/**
 * Encodes @c value as a JSON object.
 */
struct json_object * LIST_TYPE_jsonFromValue(const LIST_TYPE value);

/**
 * Produces a brief human-readable summary of @c value.
 */
char * LIST_TYPE_summaryFromValue(const LIST_TYPE value);

/// @{
/**
 * Automatically generated function.
 */
LIST_TYPE LIST_TYPE_valueFromString(const char *str);
char * LIST_TYPE_stringFromValue(const LIST_TYPE value);
/// @}

/**
 * @}
 */

#endif
