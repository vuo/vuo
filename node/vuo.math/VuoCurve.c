/**
 * @file
 * VuoCurve implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoCurve.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Curve",
					 "description" : "Type of curve.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoCurve
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoCurve.
 */
VuoCurve VuoCurve_valueFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoCurve curve = VuoCurve_Linear;

	if (strcmp(valueAsString, "linear") == 0) {
		curve = VuoCurve_Linear;
	} else if (strcmp(valueAsString, "sine") == 0) {
		curve = VuoCurve_Sine;
	} else if (strcmp(valueAsString, "gaussian") == 0) {
		curve = VuoCurve_Window_Gaussian;
	} else if (strcmp(valueAsString, "planck") == 0) {
		curve = VuoCurve_Window_Planck;
	} else if (strcmp(valueAsString, "kaiser") == 0) {
		curve = VuoCurve_Window_Kaiser;
	} else if (strcmp(valueAsString, "dolph-chebyshev") == 0) {
		curve = VuoCurve_Window_DolphChebyshev;
	} else if (strcmp(valueAsString, "poisson") == 0) {
		curve = VuoCurve_Window_Poisson;
	} else if (strcmp(valueAsString, "lanczos") == 0) {
		curve = VuoCurve_Window_Lanczos;
	} else if (strcmp(valueAsString, "parzen") == 0) {
		curve = VuoCurve_Window_Parzen;
	} else if (strcmp(valueAsString, "quadratic-in") == 0) {
		curve = VuoCurve_Quadratic_In;
	} else if (strcmp(valueAsString, "quadratic-out") == 0) {
		curve = VuoCurve_Quadratic_Out;
	} else if (strcmp(valueAsString, "quadratic-inout") == 0) {
		curve = VuoCurve_Quadratic_InOut;
	} else if (strcmp(valueAsString, "quadratic-outin") == 0) {
		curve = VuoCurve_Quadratic_OutIn;
	} else if (strcmp(valueAsString, "exponential-in") == 0) {
		curve = VuoCurve_Exponential_In;
	} else if (strcmp(valueAsString, "exponential-out") == 0) {
		curve = VuoCurve_Exponential_Out;
	} else if (strcmp(valueAsString, "exponential-inout") == 0) {
		curve = VuoCurve_Exponential_InOut;
	} else if (strcmp(valueAsString, "exponential-outin") == 0) {
		curve = VuoCurve_Exponential_OutIn;
	} else if (strcmp(valueAsString, "circular-in") == 0) {
		curve = VuoCurve_Circular_In;
	} else if (strcmp(valueAsString, "circular-out") == 0) {
		curve = VuoCurve_Circular_Out;
	} else if (strcmp(valueAsString, "circular-inout") == 0) {
		curve = VuoCurve_Circular_InOut;
	} else if (strcmp(valueAsString, "circular-outin") == 0) {
		curve = VuoCurve_Circular_OutIn;
	} else if (strcmp(valueAsString, "spring-in") == 0) {
		curve = VuoCurve_Spring_In;
	} else if (strcmp(valueAsString, "spring-out") == 0) {
		curve = VuoCurve_Spring_Out;
	} else if (strcmp(valueAsString, "spring-inout") == 0) {
		curve = VuoCurve_Spring_InOut;
	} else if (strcmp(valueAsString, "spring-outin") == 0) {
		curve = VuoCurve_Spring_OutIn;
	}

	return curve;
}

/**
 * @ingroup VuoCurve
 * Encodes @c value as a JSON object.
 */
json_object * VuoCurve_jsonFromValue(const VuoCurve value)
{
	char * valueAsString = "";

	switch (value) {
		case VuoCurve_Linear:
			valueAsString = "linear";
			break;
		case VuoCurve_Sine:
			valueAsString = "sine";
			break;
		case VuoCurve_Window_Gaussian:
			valueAsString = "gaussian";
			break;
		case VuoCurve_Window_Planck:
			valueAsString = "planck";
			break;
		case VuoCurve_Window_Kaiser:
			valueAsString = "kaiser";
			break;
		case VuoCurve_Window_DolphChebyshev:
			valueAsString = "dolph-chebyshev";
			break;
		case VuoCurve_Window_Poisson:
			valueAsString = "poisson";
			break;
		case VuoCurve_Window_Lanczos:
			valueAsString = "lanczos";
			break;
		case VuoCurve_Window_Parzen:
			valueAsString = "parzen";
			break;
		case VuoCurve_Quadratic_In:
			valueAsString = "quadratic-in";
			break;
		case VuoCurve_Quadratic_Out:
			valueAsString = "quadratic-out";
			break;
		case VuoCurve_Quadratic_InOut:
			valueAsString = "quadratic-inout";
			break;
		case VuoCurve_Quadratic_OutIn:
			valueAsString = "quadratic-outin";
			break;
		case VuoCurve_Exponential_In:
			valueAsString = "exponential-in";
			break;
		case VuoCurve_Exponential_Out:
			valueAsString = "exponential-out";
			break;
		case VuoCurve_Exponential_InOut:
			valueAsString = "exponential-inout";
			break;
		case VuoCurve_Exponential_OutIn:
			valueAsString = "exponential-outin";
			break;
		case VuoCurve_Circular_In:
			valueAsString = "circular-in";
			break;
		case VuoCurve_Circular_Out:
			valueAsString = "circular-out";
			break;
		case VuoCurve_Circular_InOut:
			valueAsString = "circular-inout";
			break;
		case VuoCurve_Circular_OutIn:
			valueAsString = "circular-outin";
			break;
		case VuoCurve_Spring_In:
			valueAsString = "spring-in";
			break;
		case VuoCurve_Spring_Out:
			valueAsString = "spring-out";
			break;
		case VuoCurve_Spring_InOut:
			valueAsString = "spring-inout";
			break;
		case VuoCurve_Spring_OutIn:
			valueAsString = "spring-outin";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * @ingroup VuoCurve
 * Same as @c %VuoCurve_stringFromValue()
 */
char * VuoCurve_summaryFromValue(const VuoCurve value)
{
	return VuoCurve_stringFromValue(value);
}
