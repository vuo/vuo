/**
 * @file
 * VuoNotePriority C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUONOTEPRIORITY_H
#define VUONOTEPRIORITY_H

/// @{
typedef void * VuoList_VuoNotePriority;
#define VuoList_VuoNotePriority_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoNotePriority VuoNotePriority
 * Specifies the algorithm for collapsing multiple simultaneously-pressed notes into a single note.
 *
 * @{
 */

/**
 * Specifies the algorithm for collapsing multiple simultaneously-pressed notes into a single note.
 */
typedef enum
{
	VuoNotePriority_First,
	VuoNotePriority_Last,
	VuoNotePriority_Lowest,
	VuoNotePriority_Highest,
} VuoNotePriority;

VuoNotePriority VuoNotePriority_valueFromJson(struct json_object * js);
struct json_object * VuoNotePriority_jsonFromValue(const VuoNotePriority value);
VuoList_VuoNotePriority VuoNotePriority_allowedValues(void);
char * VuoNotePriority_summaryFromValue(const VuoNotePriority value);

/**
 * Automatically generated function.
 */
///@{
VuoNotePriority VuoNotePriority_valueFromString(const char *str);
char * VuoNotePriority_stringFromValue(const VuoNotePriority value);
void VuoNotePriority_retain(VuoNotePriority value);
void VuoNotePriority_release(VuoNotePriority value);
///@}

/**
 * @}
 */

#endif // VUONOTEPRIORITY_H
