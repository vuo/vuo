/**
 * @file
 * VuoCurveEasing C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCURVEEASING_H
#define VUOCURVEEASING_H

/// @{
typedef const struct VuoList_VuoCurveEasing_struct { void *l; } * VuoList_VuoCurveEasing;
#define VuoList_VuoCurveEasing_TYPE_DEFINED
/// @}

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

VuoCurveEasing VuoCurveEasing_makeFromJson(struct json_object * js);
struct json_object * VuoCurveEasing_getJson(const VuoCurveEasing value);
VuoList_VuoCurveEasing VuoCurveEasing_getAllowedValues(void);
char * VuoCurveEasing_getSummary(const VuoCurveEasing value);

/// @{
/**
 * Automatically generated function.
 */
VuoCurveEasing VuoCurveEasing_makeFromString(const char *str);
char * VuoCurveEasing_getString(const VuoCurveEasing value);
void VuoCurveEasing_retain(VuoCurveEasing value);
void VuoCurveEasing_release(VuoCurveEasing value);
/// @}

/**
 * @}
*/

#endif
