/**
 * @file
 * VuoCursor C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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

VuoCursor VuoCursor_makeFromJson(struct json_object * js);
struct json_object * VuoCursor_getJson(const VuoCursor value);
VuoList_VuoCursor VuoCursor_getAllowedValues(void);
char * VuoCursor_getSummary(const VuoCursor value);

bool VuoCursor_isPopulated(const VuoCursor value);

/**
 * Automatically generated function.
 */
///@{
VuoCursor VuoCursor_makeFromString(const char *str);
char * VuoCursor_getString(const VuoCursor value);
void VuoCursor_retain(VuoCursor value);
void VuoCursor_release(VuoCursor value);
///@}

/**
 * @}
 */

#endif // VUOCURSOR_H
