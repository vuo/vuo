/**
 * @file
 * VuoCursor C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCURSOR_H
#define VUOCURSOR_H

/// @{
typedef const struct VuoList_VuoCursor_struct { void *l; } * VuoList_VuoCursor;
#define VuoList_VuoCursor_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoCursor VuoCursor
 * A mouse cursor.
 *
 * @{
 */

/**
 * A mouse cursor image.
 */
typedef enum
{
	VuoCursor_None,
	VuoCursor_Pointer,
	VuoCursor_Crosshair,
	VuoCursor_HandOpen,
	VuoCursor_HandClosed,
	VuoCursor_IBeam,
	VuoCursor_Circle
} VuoCursor;

VuoCursor VuoCursor_valueFromJson(struct json_object * js);
struct json_object * VuoCursor_jsonFromValue(const VuoCursor value);
VuoList_VuoCursor VuoCursor_allowedValues(void);
char * VuoCursor_summaryFromValue(const VuoCursor value);

/**
 * Automatically generated function.
 */
///@{
VuoCursor VuoCursor_valueFromString(const char *str);
char * VuoCursor_stringFromValue(const VuoCursor value);
void VuoCursor_retain(VuoCursor value);
void VuoCursor_release(VuoCursor value);
///@}

/**
 * @}
 */

#endif // VUOCURSOR_H
