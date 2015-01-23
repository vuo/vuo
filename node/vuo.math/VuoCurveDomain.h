/**
 * @file
 * VuoCurveDomain C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCURVEDOMAIN_H
#define VUOCURVEDOMAIN_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoCurveDomain VuoCurveDomain
 * An enum defining different types of curves.
 *
 * @{
 */

/**
 * An enum defining different types of domains that can be applied to curves.
 */
typedef enum {
	VuoCurveDomain_Clamp,
	VuoCurveDomain_Infinite,
	VuoCurveDomain_Wrap,
	VuoCurveDomain_Mirror
} VuoCurveDomain;

VuoCurveDomain VuoCurveDomain_valueFromJson(struct json_object * js);
struct json_object * VuoCurveDomain_jsonFromValue(const VuoCurveDomain value);
char * VuoCurveDomain_summaryFromValue(const VuoCurveDomain value);

/// @{
/**
 * Automatically generated function.
 */
VuoCurveDomain VuoCurveDomain_valueFromString(const char *str);
char * VuoCurveDomain_stringFromValue(const VuoCurveDomain value);
/// @}

/**
 * @}
*/

#endif
