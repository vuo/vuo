/**
 * @file
 * VuoCurve implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoCurve.h"
#include "VuoList_VuoCurve.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Curve",
					 "description" : "Type of curve.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoCurve",
						"VuoCurveEasing",
						"VuoLoopType",
						"VuoPoint2d",
						"VuoPoint3d",
						"VuoReal"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoCurve
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoCurve.
 */
VuoCurve VuoCurve_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	if (strcmp(valueAsString, "quadratic") == 0)
		return VuoCurve_Quadratic;
	else if (strcmp(valueAsString, "cubic") == 0)
		return VuoCurve_Cubic;
	else if (strcmp(valueAsString, "circular") == 0)
		return VuoCurve_Circular;
	else if (strcmp(valueAsString, "exponential") == 0)
		return VuoCurve_Exponential;

	return VuoCurve_Linear;
}

/**
 * @ingroup VuoCurve
 * Encodes @c value as a JSON object.
 */
json_object * VuoCurve_getJson(const VuoCurve value)
{
	char *valueAsString = "linear";

	if (value == VuoCurve_Quadratic)
		valueAsString = "quadratic";
	else if (value == VuoCurve_Cubic)
		valueAsString = "cubic";
	else if (value == VuoCurve_Circular)
		valueAsString = "circular";
	else if (value == VuoCurve_Exponential)
		valueAsString = "exponential";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoCurve VuoCurve_getAllowedValues(void)
{
	VuoList_VuoCurve l = VuoListCreate_VuoCurve();
	VuoListAppendValue_VuoCurve(l, VuoCurve_Linear);
	VuoListAppendValue_VuoCurve(l, VuoCurve_Quadratic);
	VuoListAppendValue_VuoCurve(l, VuoCurve_Cubic);
	VuoListAppendValue_VuoCurve(l, VuoCurve_Circular);
	VuoListAppendValue_VuoCurve(l, VuoCurve_Exponential);
	return l;
}

/**
 * @ingroup VuoCurve
 * Same as @c %VuoCurve_getString()
 */
char * VuoCurve_getSummary(const VuoCurve value)
{
	char *valueAsString = "Linear";

	if (value == VuoCurve_Quadratic)
		valueAsString = "Quadratic";
	else if (value == VuoCurve_Cubic)
		valueAsString = "Cubic";
	else if (value == VuoCurve_Circular)
		valueAsString = "Circular";
	else if (value == VuoCurve_Exponential)
		valueAsString = "Exponential";

	return strdup(valueAsString);
}

/**
 * Returns an interpolated value between @c startPosition and @c endPosition.
 */
VuoReal VuoReal_curve(VuoReal time, VuoReal startPosition, VuoReal endPosition, VuoReal duration, VuoCurve curve, VuoCurveEasing easing, VuoLoopType loop)
{
	VuoReal normalizedTime = MIN(MAX(time/duration,0.),1.);
	if (loop == VuoLoopType_Loop)
	{
		normalizedTime = fmod(time/duration, 1);
		if (time < 0)
			normalizedTime = 1. + normalizedTime;
	}
	else if (loop == VuoLoopType_Mirror)
	{
		normalizedTime = fmod(time/duration, 2);
		if (time < 0)
			normalizedTime = 2. + normalizedTime;
		if (normalizedTime > 1)
			normalizedTime = 2. - normalizedTime;
	}

	VuoReal x = normalizedTime;
	if (easing == VuoCurveEasing_Out)
		x = normalizedTime - 1.;
	else if (easing == VuoCurveEasing_InOut)
	{
		if (normalizedTime < 0.5)
			x = normalizedTime*2.;
		else
			x = (normalizedTime-1.)*2.;
	}
	else if (easing == VuoCurveEasing_Middle)
	{
		if (normalizedTime < 0.5)
			x = (normalizedTime-0.5)*2.;
		else
			x = (normalizedTime-0.5)*2.;
	}

	VuoReal normalizedValue = x;
	if (curve == VuoCurve_Linear)
		normalizedValue = fabs(x);
	else if (curve == VuoCurve_Quadratic)
		normalizedValue = pow(x,2);
	else if (curve == VuoCurve_Cubic)
		normalizedValue = pow(fabs(x),3);
	else if (curve == VuoCurve_Circular)
		normalizedValue = -(sqrt(1.-pow(x,2))-1.);
	else if (curve == VuoCurve_Exponential)
		normalizedValue = pow(2,10.*(fabs(x)-1.));

	if (easing == VuoCurveEasing_Out)
		normalizedValue = 1. - normalizedValue;
	else if (easing == VuoCurveEasing_InOut)
	{
		if (normalizedTime < 0.5)
			normalizedValue /= 2.;
		else
			normalizedValue = 1 - normalizedValue/2.;
	}
	else if (easing == VuoCurveEasing_Middle)
	{
		if (normalizedTime < 0.5)
			normalizedValue = (1.-normalizedValue)/2.;
		else
			normalizedValue = (normalizedValue+1.)/2.;
	}

	return normalizedValue*(endPosition-startPosition) + startPosition;
}

/**
 * Returns an interpolated value between @c startPosition and @c endPosition.
 */
VuoPoint2d VuoPoint2d_curve(VuoReal time, VuoPoint2d startPosition, VuoPoint2d endPosition, VuoReal duration, VuoCurve curve, VuoCurveEasing easing, VuoLoopType loop)
{
	VuoPoint2d p;
	p.x = VuoReal_curve(time, startPosition.x, endPosition.x, duration, curve, easing, loop);
	p.y = VuoReal_curve(time, startPosition.y, endPosition.y, duration, curve, easing, loop);
	return p;
}

/**
 * Returns an interpolated value between @c startPosition and @c endPosition.
 */
VuoPoint3d VuoPoint3d_curve(VuoReal time, VuoPoint3d startPosition, VuoPoint3d endPosition, VuoReal duration, VuoCurve curve, VuoCurveEasing easing, VuoLoopType loop)
{
	VuoPoint3d p;
	p.x = VuoReal_curve(time, startPosition.x, endPosition.x, duration, curve, easing, loop);
	p.y = VuoReal_curve(time, startPosition.y, endPosition.y, duration, curve, easing, loop);
	p.z = VuoReal_curve(time, startPosition.z, endPosition.z, duration, curve, easing, loop);
	return p;
}
