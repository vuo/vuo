/**
 * @file
 * VuoCurveEasing C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCURVEEASING_H
#define VUOCURVEEASING_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoCurveEasing VuoCurveEasing
 * Specifies which part of a curve is eased.
 *
 * @{
 */

/**
 * Specifies which part of a curve is eased.
 */
typedef enum {
	VuoCurveEasing_In,
	VuoCurveEasing_Out,
	VuoCurveEasing_InOut,
	VuoCurveEasing_Middle
} VuoCurveEasing;

VuoCurveEasing VuoCurveEasing_valueFromJson(struct json_object * js);
struct json_object * VuoCurveEasing_jsonFromValue(const VuoCurveEasing value);
char * VuoCurveEasing_summaryFromValue(const VuoCurveEasing value);

/// @{
/**
 * Automatically generated function.
 */
VuoCurveEasing VuoCurveEasing_valueFromString(const char *str);
char * VuoCurveEasing_stringFromValue(const VuoCurveEasing value);
/// @}

/**
 * @}
*/

#endif
