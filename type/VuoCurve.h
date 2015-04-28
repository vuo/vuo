/**
 * @file
 * VuoCurve C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCURVE_H
#define VUOCURVE_H

#include "VuoReal.h"
#include "VuoPoint2d.h"
#include "VuoPoint3d.h"
#include "VuoCurveEasing.h"
#include "VuoLoopType.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoCurve VuoCurve
 * An enum defining different types of curves.
 *
 * @{
 */

/**
 * An enum defining different types of curves.
 */
typedef enum {
	VuoCurve_Linear,
	VuoCurve_Quadratic,
	VuoCurve_Cubic,
	VuoCurve_Circular,
	VuoCurve_Exponential
} VuoCurve;

VuoCurve VuoCurve_valueFromJson(struct json_object * js);
struct json_object * VuoCurve_jsonFromValue(const VuoCurve value);
char * VuoCurve_summaryFromValue(const VuoCurve value);

/// @{
/**
 * Automatically generated function.
 */
VuoCurve VuoCurve_valueFromString(const char *str);
char * VuoCurve_stringFromValue(const VuoCurve value);
/// @}

VuoReal		VuoReal_curve(		VuoReal time, VuoReal startPosition,	VuoReal endPosition,	VuoReal duration, VuoCurve curve, VuoCurveEasing easing, VuoLoopType loop);
VuoPoint2d	VuoPoint2d_curve(	VuoReal time, VuoPoint2d startPosition,	VuoPoint2d endPosition,	VuoReal duration, VuoCurve curve, VuoCurveEasing easing, VuoLoopType loop);
VuoPoint3d	VuoPoint3d_curve(	VuoReal time, VuoPoint3d startPosition,	VuoPoint3d endPosition,	VuoReal duration, VuoCurve curve, VuoCurveEasing easing, VuoLoopType loop);

/**
 * @}
*/

#endif
