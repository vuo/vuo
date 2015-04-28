/**
 * @file
 * VuoPointsParametric implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
						 "muParser"
					 ]
				 });
#endif
}

/// Constant for converting degrees to radians
#define DEG2RAD 0.0174532925
/// Constant for converting radians to degrees
#define RAD2DEG 57.2957795
/// Constant providing the ratio of a circle's circumference to its diameter
#define PI 3.14159265359


/**
 * Generates a mesh (@c VuoVertices) given a set of mathematical expressions specifying a warped surface.
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

	xParser.DefineConst("DEG2RAD", (double)DEG2RAD);
	yParser.DefineConst("DEG2RAD", (double)DEG2RAD);
	zParser.DefineConst("DEG2RAD", (double)DEG2RAD);

	xParser.DefineConst("RAD2DEG", (double)RAD2DEG);
	yParser.DefineConst("RAD2DEG", (double)RAD2DEG);
	zParser.DefineConst("RAD2DEG", (double)RAD2DEG);

	xParser.DefineConst("PI", (double)PI);
	yParser.DefineConst("PI", (double)PI);
	zParser.DefineConst("PI", (double)PI);

	VuoList_VuoPoint3d points = VuoListCreate_VuoPoint3d();

	try
	{
		for(int x = 0; x < subdivisions; x++)
		{
			uVar = VuoReal_lerp(uMin, uMax, x/(float)(subdivisions-1.));

			VuoPoint3d p = VuoPoint3d_make(
				xParser.Eval(),
				yParser.Eval(),
				zParser.Eval());
			VuoListAppendValue_VuoPoint3d(points, p);
		}
	}
	catch (mu::Parser::exception_type &e)
	{
		fprintf(stderr, "VuoPointsParametric_generate() Error: %s\n", e.GetMsg().c_str());
		return VuoListCreate_VuoPoint3d();
	}
	return points;
}

/**
 * Generates a mesh (@c VuoVertices) given a set of mathematical expressions specifying a warped surface.
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

	xParser.DefineConst("DEG2RAD", (double)DEG2RAD);
	yParser.DefineConst("DEG2RAD", (double)DEG2RAD);
	zParser.DefineConst("DEG2RAD", (double)DEG2RAD);

	xParser.DefineConst("RAD2DEG", (double)RAD2DEG);
	yParser.DefineConst("RAD2DEG", (double)RAD2DEG);
	zParser.DefineConst("RAD2DEG", (double)RAD2DEG);

	xParser.DefineConst("PI", (double)PI);
	yParser.DefineConst("PI", (double)PI);
	zParser.DefineConst("PI", (double)PI);

	VuoList_VuoPoint3d points = VuoListCreate_VuoPoint3d();

	try
	{
		for(int y = 0; y < rows; y++)
		{
			vVar = VuoReal_lerp(vMin, vMax, y/(float)(rows-1));

			for(int x = 0; x < columns; x++)
			{
				uVar = VuoReal_lerp(uMin, uMax, x/(float)(columns-1.));

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
		fprintf(stderr, "VuoPointsParametric_generate() Error: %s\n", e.GetMsg().c_str());
		return VuoListCreate_VuoPoint3d();
	}
	return points;
}
