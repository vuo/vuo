/**
 * @file
 * vuo.boolean C type definition.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOBOOLEAN_H
#define VUOBOOLEAN_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoBoolean VuoBoolean
 * A Boolean.
 *
 * @{
 */

/**
 * A Boolean
 */
typedef signed long VuoBoolean;

VuoBoolean VuoBoolean_valueFromJson(struct json_object * js);
struct json_object * VuoBoolean_jsonFromValue(const VuoBoolean value);
char * VuoBoolean_summaryFromValue(const VuoBoolean value);

/// @{
/**
 * Automatically generated function.
 */
VuoBoolean VuoBoolean_valueFromString(const char *str);
char * VuoBoolean_stringFromValue(const VuoBoolean value);
/// @}

/**
 * @}
 */

#endif
