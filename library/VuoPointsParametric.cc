/**
 * @file
 * VuoPointsParametric implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoPointsParametric.h"
#include "VuoMathExpressionParser.h"
#include <muParser/muParser.h>

extern "C"
{
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
						 "muparser"
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
	if (subdivisions <= 0 || VuoText_isEmpty(xExp) || VuoText_isEmpty(yExp) || VuoText_isEmpty(zExp))
		return VuoListCreate_VuoPoint3d();

	mu::Parser xParser, yParser, zParser;

	xParser.SetExpr(xExp);
	yParser.SetExpr(yExp);
	zParser.SetExpr(zExp);

	mu::value_type uVar = 0;

	xParser.DefineVar("u", &uVar); xParser.DefineVar("U", &uVar);
	yParser.DefineVar("u", &uVar); yParser.DefineVar("U", &uVar);
	zParser.DefineVar("u", &uVar); zParser.DefineVar("U", &uVar);

	mu::value_type iVar = 0;

	xParser.DefineVar("i", &iVar); xParser.DefineVar("I", &iVar);
	yParser.DefineVar("i", &iVar); yParser.DefineVar("I", &iVar);
	zParser.DefineVar("i", &iVar); zParser.DefineVar("I", &iVar);

	xParser.DefineConst("time", (double)time); xParser.DefineConst("Time", (double)time); xParser.DefineConst("TIME", (double)time);
	yParser.DefineConst("time", (double)time); yParser.DefineConst("Time", (double)time); yParser.DefineConst("TIME", (double)time);
	zParser.DefineConst("time", (double)time); zParser.DefineConst("Time", (double)time); zParser.DefineConst("TIME", (double)time);

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

			iVar = x + 1;

			pointsArray[x] = (VuoPoint3d){
				(float)xParser.Eval(),
				(float)yParser.Eval(),
				(float)zParser.Eval()};
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
	if (rows <= 0 || columns <= 0 || VuoText_isEmpty(xExp) || VuoText_isEmpty(yExp) || VuoText_isEmpty(zExp))
		return VuoListCreate_VuoPoint3d();

	mu::Parser xParser, yParser, zParser;

	xParser.SetExpr(xExp);
	yParser.SetExpr(yExp);
	zParser.SetExpr(zExp);

	mu::value_type uVar = 0;
	mu::value_type vVar = 0;

	xParser.DefineVar("u", &uVar); xParser.DefineVar("U", &uVar);
	yParser.DefineVar("u", &uVar); yParser.DefineVar("U", &uVar);
	zParser.DefineVar("u", &uVar); zParser.DefineVar("U", &uVar);
	xParser.DefineVar("v", &vVar); xParser.DefineVar("V", &vVar);
	yParser.DefineVar("v", &vVar); yParser.DefineVar("V", &vVar);
	zParser.DefineVar("v", &vVar); zParser.DefineVar("V", &vVar);

	mu::value_type iVar = 0;
	mu::value_type jVar = 0;

	xParser.DefineVar("i", &iVar); xParser.DefineVar("I", &iVar);
	yParser.DefineVar("i", &iVar); yParser.DefineVar("I", &iVar);
	zParser.DefineVar("i", &iVar); zParser.DefineVar("I", &iVar);
	xParser.DefineVar("j", &jVar); xParser.DefineVar("J", &jVar);
	yParser.DefineVar("j", &jVar); yParser.DefineVar("J", &jVar);
	zParser.DefineVar("j", &jVar); zParser.DefineVar("J", &jVar);

	xParser.DefineConst("time", (double)time); xParser.DefineConst("Time", (double)time); xParser.DefineConst("TIME", (double)time);
	yParser.DefineConst("time", (double)time); yParser.DefineConst("Time", (double)time); yParser.DefineConst("TIME", (double)time);
	zParser.DefineConst("time", (double)time); zParser.DefineConst("Time", (double)time); zParser.DefineConst("TIME", (double)time);

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

				iVar = x + 1;
				jVar = y + 1;

				pointsArray[y*columns + x] = (VuoPoint3d){
					(float)xParser.Eval(),
					(float)yParser.Eval(),
					(float)zParser.Eval()};
			}
		}
	}
	catch (mu::Parser::exception_type &e)
	{
		VUserLog("Error: %s", e.GetMsg().c_str());
	}
	return points;
}
