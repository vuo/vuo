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
	VuoCurve_Sine,
	VuoCurve_Window_Gaussian,
	VuoCurve_Window_Planck,
	VuoCurve_Window_Kaiser,
	VuoCurve_Window_DolphChebyshev,
	VuoCurve_Window_Poisson,
	VuoCurve_Window_Lanczos,
	VuoCurve_Window_Parzen,
	VuoCurve_Quadratic_In,
	VuoCurve_Quadratic_Out,
	VuoCurve_Quadratic_InOut,
	VuoCurve_Quadratic_OutIn,
	VuoCurve_Exponential_In,
	VuoCurve_Exponential_Out,
	VuoCurve_Exponential_InOut,
	VuoCurve_Exponential_OutIn,
	VuoCurve_Circular_In,
	VuoCurve_Circular_Out,
	VuoCurve_Circular_InOut,
	VuoCurve_Circular_OutIn,
	VuoCurve_Spring_In,
	VuoCurve_Spring_Out,
	VuoCurve_Spring_InOut,
	VuoCurve_Spring_OutIn

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

/**
 * @}
*/

#endif
