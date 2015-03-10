/**
 * @file
 * VuoLeapTouchZone C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOLEAPTOUCHZONE_H
#define VUOLEAPTOUCHZONE_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoLeapTouchZone VuoLeapTouchZone
 * Defines the type of object that a VuoLeapPointable is representing.
 *
 * @{
 */

/**
 * An enum defining different types of blend shaders.
 */
typedef enum {
	VuoLeapTouchZone_None,
	VuoLeapTouchZone_Hovering,
	VuoLeapTouchZone_Touching
} VuoLeapTouchZone;

VuoLeapTouchZone VuoLeapTouchZone_valueFromJson(struct json_object * js);
struct json_object * VuoLeapTouchZone_jsonFromValue(const VuoLeapTouchZone value);
char * VuoLeapTouchZone_summaryFromValue(const VuoLeapTouchZone value);

/// @{
/**
 * Automatically generated function.
 */
VuoLeapTouchZone VuoLeapTouchZone_valueFromString(const char *str);
char * VuoLeapTouchZone_stringFromValue(const VuoLeapTouchZone value);
/// @}

/**
 * @}
*/

#endif
