/**
 * @file
 * VuoReal C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOREAL_H
#define VUOREAL_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoReal VuoReal
 * A floating-point number.
 *
 * @{
 */

/**
 * A floating-point number.
 */
typedef double VuoReal;

VuoReal VuoReal_valueFromJson(struct json_object *js);
struct json_object * VuoReal_jsonFromValue(const VuoReal value);
char * VuoReal_summaryFromValue(const VuoReal value);

VuoReal VuoReal_min(VuoReal *terms, unsigned long termsCount);
VuoReal VuoReal_max(VuoReal *terms, unsigned long termsCount);

/// @{
/**
 * Automatically generated function.
 */
VuoReal VuoReal_valueFromString(const char *str);
char * VuoReal_stringFromValue(const VuoReal value);
/// @}

/**
 * @}
 */

#endif
