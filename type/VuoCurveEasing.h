/**
 * @file
 * VuoCurveEasing C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoCurveEasing_h
#define VuoCurveEasing_h

#ifdef __cplusplus
extern "C" {
#endif

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

#include "VuoList_VuoCurveEasing.h"

VuoCurveEasing VuoCurveEasing_makeFromJson(struct json_object * js);
struct json_object * VuoCurveEasing_getJson(const VuoCurveEasing value);
VuoList_VuoCurveEasing VuoCurveEasing_getAllowedValues(void);
char * VuoCurveEasing_getSummary(const VuoCurveEasing value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoCurveEasing_getString(const VuoCurveEasing value);
void VuoCurveEasing_retain(VuoCurveEasing value);
void VuoCurveEasing_release(VuoCurveEasing value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif

#endif
