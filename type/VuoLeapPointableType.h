/**
 * @file
 * VuoLeapPointableType C type definition.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOLEAPPOINTABLETYPE_H
#define VUOLEAPPOINTABLETYPE_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoLeapPointableType VuoLeapPointableType
 * Defines the type of object that a VuoLeapPointable is representing.
 *
 * @{
 */

/**
 * An enum defining different types of blend shaders.
 */
typedef enum {
	VuoLeapPointableType_Finger,
	VuoLeapPointableType_Tool
} VuoLeapPointableType;

VuoLeapPointableType VuoLeapPointableType_valueFromJson(struct json_object * js);
struct json_object * VuoLeapPointableType_jsonFromValue(const VuoLeapPointableType value);
char * VuoLeapPointableType_summaryFromValue(const VuoLeapPointableType value);

/// @{
/**
 * Automatically generated function.
 */
VuoLeapPointableType VuoLeapPointableType_valueFromString(const char *str);
char * VuoLeapPointableType_stringFromValue(const VuoLeapPointableType value);
/// @}

/**
 * @}
*/

#endif
