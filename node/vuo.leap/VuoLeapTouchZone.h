/**
 * @file
 * VuoLeapTouchZone C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOLEAPTOUCHZONE_H
#define VUOLEAPTOUCHZONE_H

/// @{
typedef const struct VuoList_VuoLeapTouchZone_struct { void *l; } * VuoList_VuoLeapTouchZone;
#define VuoList_VuoLeapTouchZone_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoLeapTouchZone VuoLeapTouchZone
 * How close a pointable is to the touch zone.
 *
 * @{
 */

/**
 * How close a pointable is to the touch zone.
 */
typedef enum {
	VuoLeapTouchZone_None,
	VuoLeapTouchZone_Hovering,
	VuoLeapTouchZone_Touching
} VuoLeapTouchZone;

VuoLeapTouchZone VuoLeapTouchZone_makeFromJson(struct json_object * js);
struct json_object * VuoLeapTouchZone_getJson(const VuoLeapTouchZone value);
VuoList_VuoLeapTouchZone VuoLeapTouchZone_getAllowedValues(void);
char * VuoLeapTouchZone_getSummary(const VuoLeapTouchZone value);

/// @{
/**
 * Automatically generated function.
 */
VuoLeapTouchZone VuoLeapTouchZone_makeFromString(const char *str);
char * VuoLeapTouchZone_getString(const VuoLeapTouchZone value);
/// @}

/**
 * @}
*/

#endif
