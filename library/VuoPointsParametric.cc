/**
 * @file
 * VuoPointsParametric implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include "VuoPointsParametric.h"
#include "VuoMathExpressionParser.h"
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
						 "VuoMathExpressionParser",
						 "muParser"
					 ]
				 });
#endif
}

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
	if (subdivisions <= 0 || !xExp || !yExp || !zExp)
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

	VuoMathExpressionParser_defineStandardLibrary(&xParser);
	VuoMathExpressionParser_defineStandardLibrary(&yParser);
	VuoMathExpressionParser_defineStandardLibrary(&zParser);

	VuoList_VuoPoint3d points = VuoListCreateWithCount_VuoPoint3d(subdivisions, (VuoPoint3d){0,0,0});
	VuoPoint3d *pointsArray = VuoListGetData_VuoPoint3d(points);

	try
	{
		for(int x = 0; x < subdivisions; x++)
		{
			if (subdivisions > 1)
				uVar = VuoReal_lerp(uMin, uMax, x/(float)(subdivisions-1.));
			else
				uVar = (uMin + uMax)/2.;

			pointsArray[x] = (VuoPoint3d){
				xParser.Eval(),
				yParser.Eval(),
				zParser.Eval()};
		}
	}
	catch (mu::Parser::exception_type &e)
	{
		VUserLog("Error: %s", e.GetMsg().c_str());
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
	if (rows <= 0 || columns <= 0 || !xExp || !yExp || !zExp)
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

	VuoMathExpressionParser_defineStandardLibrary(&xParser);
	VuoMathExpressionParser_defineStandardLibrary(&yParser);
	VuoMathExpressionParser_defineStandardLibrary(&zParser);

	VuoList_VuoPoint3d points = VuoListCreateWithCount_VuoPoint3d(rows*columns, (VuoPoint3d){0,0,0});
	VuoPoint3d *pointsArray = VuoListGetData_VuoPoint3d(points);

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

				pointsArray[y*columns + x] = (VuoPoint3d){
					xParser.Eval(),
					yParser.Eval(),
					zParser.Eval()};
			}
		}
	}
	catch (mu::Parser::exception_type &e)
	{
		VUserLog("Error: %s", e.GetMsg().c_str());
	}
	return points;
}
