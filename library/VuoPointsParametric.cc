/**
 * @file
 * VuoPointsParametric implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include "VuoPointsParametric.h"
#include "muParser.h"


extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoPointsParametric",
					 "dependencies" : [
						 "VuoInteger",
						 "VuoPoint3d",
						 "VuoReal",
						 "VuoText",
						 "VuoList_VuoPoint3d",
						 "muParser"
					 ]
				 });
#endif
}

/// Constant providing the ratio of a circle's circumference to its diameter
#define PI 3.14159265359

/**
 * Converts degrees to radians.
 */
static double deg2rad(double degrees)
{
	return degrees * 0.0174532925;
}

/**
 * Converts radians to degrees.
 */
static double rad2deg(double radians)
{
	return radians * 57.2957795;
}

/// @{
/**
 * Trigonometric functions that take degrees instead of radians.
 */
static double sinInDegrees(double degrees) { return sin(deg2rad(degrees)); }
static double cosInDegrees(double degrees) { return cos(deg2rad(degrees)); }
static double tanInDegrees(double degrees) { return tan(deg2rad(degrees)); }
static double asinInDegrees(double x) { return rad2deg(asin(x)); }
static double acosInDegrees(double x) { return rad2deg(acos(x)); }
static double atanInDegrees(double x) { return rad2deg(atan(x)); }
static double sinhInDegrees(double degrees) { return sinh(deg2rad(degrees)); }
static double coshInDegrees(double degrees) { return cosh(deg2rad(degrees)); }
static double tanhInDegrees(double degrees) { return tanh(deg2rad(degrees)); }
static double asinhInDegrees(double x) { return rad2deg(asinh(x)); }
static double acoshInDegrees(double x) { return rad2deg(acosh(x)); }
static double atanhInDegrees(double x) { return rad2deg(atanh(x)); }
/// @}

/**
 * Generates a mesh given a set of mathematical expressions specifying a warped surface.
 */
VuoList_VuoPoint3d VuoPointsParametric1d_generate(
	VuoReal time,
	VuoText xExp,
	VuoText yExp,
	VuoText zExp,
	VuoInteger subdivisions,
	VuoReal uMin,
	VuoReal uMax)
{
	if (subdivisions<=0)
		return VuoListCreate_VuoPoint3d();

	mu::Parser xParser, yParser, zParser;

	xParser.SetExpr(xExp);
	yParser.SetExpr(yExp);
	zParser.SetExpr(zExp);

	mu::value_type uVar = 0;

	xParser.DefineVar("u", &uVar);
	yParser.DefineVar("u", &uVar);
	zParser.DefineVar("u", &uVar);

	xParser.DefineConst("time", (double)time);
	yParser.DefineConst("time", (double)time);
	zParser.DefineConst("time", (double)time);

	xParser.DefineFun("deg2rad", deg2rad, false);
	yParser.DefineFun("deg2rad", deg2rad, false);
	zParser.DefineFun("deg2rad", deg2rad, false);

	xParser.DefineFun("rad2deg", rad2deg, false);
	yParser.DefineFun("rad2deg", rad2deg, false);
	zParser.DefineFun("rad2deg", rad2deg, false);

	xParser.DefineConst("PI", (double)PI);
	yParser.DefineConst("PI", (double)PI);
	zParser.DefineConst("PI", (double)PI);

	xParser.DefineOprt("%", fmod, mu::prMUL_DIV, mu::oaLEFT, true);
	yParser.DefineOprt("%", fmod, mu::prMUL_DIV, mu::oaLEFT, true);
	zParser.DefineOprt("%", fmod, mu::prMUL_DIV, mu::oaLEFT, true);

	xParser.DefineFun("sin", sinInDegrees, false);
	xParser.DefineFun("cos", cosInDegrees, false);
	xParser.DefineFun("tan", tanInDegrees, false);
	xParser.DefineFun("asin", asinInDegrees, false);
	xParser.DefineFun("acos", acosInDegrees, false);
	xParser.DefineFun("atan", atanInDegrees, false);
	xParser.DefineFun("sinh", sinhInDegrees, false);
	xParser.DefineFun("cosh", coshInDegrees, false);
	xParser.DefineFun("tanh", tanhInDegrees, false);
	xParser.DefineFun("asinh", asinhInDegrees, false);
	xParser.DefineFun("acosh", acoshInDegrees, false);
	xParser.DefineFun("atanh", atanhInDegrees, false);
	yParser.DefineFun("sin", sinInDegrees, false);
	yParser.DefineFun("cos", cosInDegrees, false);
	yParser.DefineFun("tan", tanInDegrees, false);
	yParser.DefineFun("asin", asinInDegrees, false);
	yParser.DefineFun("acos", acosInDegrees, false);
	yParser.DefineFun("atan", atanInDegrees, false);
	yParser.DefineFun("sinh", sinhInDegrees, false);
	yParser.DefineFun("cosh", coshInDegrees, false);
	yParser.DefineFun("tanh", tanhInDegrees, false);
	yParser.DefineFun("asinh", asinhInDegrees, false);
	yParser.DefineFun("acosh", acoshInDegrees, false);
	yParser.DefineFun("atanh", atanhInDegrees, false);
	zParser.DefineFun("sin", sinInDegrees, false);
	zParser.DefineFun("cos", cosInDegrees, false);
	zParser.DefineFun("tan", tanInDegrees, false);
	zParser.DefineFun("asin", asinInDegrees, false);
	zParser.DefineFun("acos", acosInDegrees, false);
	zParser.DefineFun("atan", atanInDegrees, false);
	zParser.DefineFun("sinh", sinhInDegrees, false);
	zParser.DefineFun("cosh", coshInDegrees, false);
	zParser.DefineFun("tanh", tanhInDegrees, false);
	zParser.DefineFun("asinh", asinhInDegrees, false);
	zParser.DefineFun("acosh", acoshInDegrees, false);
	zParser.DefineFun("atanh", atanhInDegrees, false);

	VuoList_VuoPoint3d points = VuoListCreate_VuoPoint3d();

	try
	{
		for(int x = 0; x < subdivisions; x++)
		{
			if (subdivisions > 1)
				uVar = VuoReal_lerp(uMin, uMax, x/(float)(subdivisions-1.));
			else
				uVar = (uMin + uMax)/2.;

			VuoPoint3d p = VuoPoint3d_make(
				xParser.Eval(),
				yParser.Eval(),
				zParser.Eval());
			VuoListAppendValue_VuoPoint3d(points, p);
		}
	}
	catch (mu::Parser::exception_type &e)
	{
		VLog("Error: %s", e.GetMsg().c_str());
	}
	return points;
}

/**
 * Generates a mesh given a set of mathematical expressions specifying a warped surface.
 */
VuoList_VuoPoint3d VuoPointsParametric2d_generate(
	VuoReal time,
	VuoText xExp,
	VuoText yExp,
	VuoText zExp,
	VuoInteger rows,
	VuoInteger columns,
	VuoReal uMin,
	VuoReal uMax,
	VuoReal vMin,
	VuoReal vMax)
{
	if (rows<=0 || columns<=0)
		return VuoListCreate_VuoPoint3d();

	mu::Parser xParser, yParser, zParser;

	xParser.SetExpr(xExp);
	yParser.SetExpr(yExp);
	zParser.SetExpr(zExp);

	mu::value_type uVar = 0;
	mu::value_type vVar = 0;

	xParser.DefineVar("u", &uVar);
	yParser.DefineVar("u", &uVar);
	zParser.DefineVar("u", &uVar);
	xParser.DefineVar("v", &vVar);
	yParser.DefineVar("v", &vVar);
	zParser.DefineVar("v", &vVar);

	xParser.DefineConst("time", (double)time);
	yParser.DefineConst("time", (double)time);
	zParser.DefineConst("time", (double)time);

	xParser.DefineFun("deg2rad", deg2rad, false);
	yParser.DefineFun("deg2rad", deg2rad, false);
	zParser.DefineFun("deg2rad", deg2rad, false);

	xParser.DefineFun("rad2deg", rad2deg, false);
	yParser.DefineFun("rad2deg", rad2deg, false);
	zParser.DefineFun("rad2deg", rad2deg, false);

	xParser.DefineConst("PI", (double)PI);
	yParser.DefineConst("PI", (double)PI);
	zParser.DefineConst("PI", (double)PI);

	xParser.DefineOprt("%", fmod, mu::prMUL_DIV, mu::oaLEFT, true);
	yParser.DefineOprt("%", fmod, mu::prMUL_DIV, mu::oaLEFT, true);
	zParser.DefineOprt("%", fmod, mu::prMUL_DIV, mu::oaLEFT, true);

	xParser.DefineFun("sin", sinInDegrees, false);
	xParser.DefineFun("cos", cosInDegrees, false);
	xParser.DefineFun("tan", tanInDegrees, false);
	xParser.DefineFun("asin", asinInDegrees, false);
	xParser.DefineFun("acos", acosInDegrees, false);
	xParser.DefineFun("atan", atanInDegrees, false);
	xParser.DefineFun("sinh", sinhInDegrees, false);
	xParser.DefineFun("cosh", coshInDegrees, false);
	xParser.DefineFun("tanh", tanhInDegrees, false);
	xParser.DefineFun("asinh", asinhInDegrees, false);
	xParser.DefineFun("acosh", acoshInDegrees, false);
	xParser.DefineFun("atanh", atanhInDegrees, false);
	yParser.DefineFun("sin", sinInDegrees, false);
	yParser.DefineFun("cos", cosInDegrees, false);
	yParser.DefineFun("tan", tanInDegrees, false);
	yParser.DefineFun("asin", asinInDegrees, false);
	yParser.DefineFun("acos", acosInDegrees, false);
	yParser.DefineFun("atan", atanInDegrees, false);
	yParser.DefineFun("sinh", sinhInDegrees, false);
	yParser.DefineFun("cosh", coshInDegrees, false);
	yParser.DefineFun("tanh", tanhInDegrees, false);
	yParser.DefineFun("asinh", asinhInDegrees, false);
	yParser.DefineFun("acosh", acoshInDegrees, false);
	yParser.DefineFun("atanh", atanhInDegrees, false);
	zParser.DefineFun("sin", sinInDegrees, false);
	zParser.DefineFun("cos", cosInDegrees, false);
	zParser.DefineFun("tan", tanInDegrees, false);
	zParser.DefineFun("asin", asinInDegrees, false);
	zParser.DefineFun("acos", acosInDegrees, false);
	zParser.DefineFun("atan", atanInDegrees, false);
	zParser.DefineFun("sinh", sinhInDegrees, false);
	zParser.DefineFun("cosh", coshInDegrees, false);
	zParser.DefineFun("tanh", tanhInDegrees, false);
	zParser.DefineFun("asinh", asinhInDegrees, false);
	zParser.DefineFun("acosh", acoshInDegrees, false);
	zParser.DefineFun("atanh", atanhInDegrees, false);

	VuoList_VuoPoint3d points = VuoListCreate_VuoPoint3d();

	try
	{
		for(int y = 0; y < rows; y++)
		{
			if (rows > 1)
				vVar = VuoReal_lerp(vMin, vMax, y/(float)(rows-1));
			else
				vVar = (vMin + vMax)/2.;

			for(int x = 0; x < columns; x++)
			{
				if (columns > 1)
					uVar = VuoReal_lerp(uMin, uMax, x/(float)(columns-1.));
				else
					uVar = (uMin + uMax)/2.;

				VuoPoint3d p = VuoPoint3d_make(
					xParser.Eval(),
					yParser.Eval(),
					zParser.Eval());
				VuoListAppendValue_VuoPoint3d(points, p);
			}
		}
	}
	catch (mu::Parser::exception_type &e)
	{
		VLog("Error: %s", e.GetMsg().c_str());
	}
	return points;
}
