/**
 * @file
 * VuoMeshParametric implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include "VuoMeshParametric.h"
#include "muParser.h"
#include <OpenGL/CGLMacro.h>
#include "VuoMeshUtility.h"

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoMeshParametric",
					 "dependencies" : [
						 "muParser",
						 "VuoMeshUtility"
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
VuoMesh VuoMeshParametric_generate(VuoReal time, VuoText xExp, VuoText yExp, VuoText zExp, VuoInteger uSubdivisions, VuoInteger vSubdivisions, bool closeU, VuoReal uMin, VuoReal uMax, bool closeV, VuoReal vMin, VuoReal vMax)
{
	if (uSubdivisions<2 || vSubdivisions<2)
		return VuoMesh_make(0);

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

	int width = uSubdivisions;
	int height = vSubdivisions;

	float u = 0., v = 0., ustep = 1./(width-1.), vstep = 1./(height-1.);

	int vertexCount = width * height;

	VuoPoint4d *positions 	= (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*vertexCount);
	VuoPoint4d *normals 	= (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*vertexCount);
	VuoPoint4d *textures 	= (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*vertexCount);

	try
	{
		int i = 0;
		for(int y = 0; y < height; y++)
		{
			vVar = VuoReal_lerp(vMin, vMax, (closeV && y==height-1) ? 0 : v);
			for(int x = 0; x < width; x++)
			{
				uVar = VuoReal_lerp(uMin, uMax, (closeU && x==width-1) ? 0 : u);

				positions[i].x = xParser.Eval();
				positions[i].y = yParser.Eval();
				positions[i].z = zParser.Eval();
				positions[i].w = 1.;

				normals[i].x = 0.;
				normals[i].y = 0.;
				normals[i].z = 0.;
				normals[i].w = 0.;

				textures[i].x = u;
				textures[i].y = v;
				textures[i].z = 0.;
				textures[i].w = 0.;

				u += ustep;
				i++;
			}

			u = 0.;
			v += vstep;
		}
	}
	catch (mu::Parser::exception_type &e)
	{
		VLog("Error: %s", e.GetMsg().c_str());
		free(positions);
		free(normals);
		free(textures);
		return VuoMesh_make(0);
	}

	// wind triangles

	width = uSubdivisions-1;
	height = vSubdivisions-1;

	unsigned int triangleCount = (width*height)*3*2;
	unsigned int *triangles = (unsigned int *)malloc(sizeof(unsigned int)*triangleCount);

	// Prepare an array to count how many neighboring face normals have been added to the vertex normal, so we can divide later to get the average.
	unsigned int* normalCount = (unsigned int*)calloc(sizeof(unsigned int) * vertexCount, sizeof(unsigned int));
	for (int i=0;i<vertexCount;++i)
		normalCount[i] = 0;

	int index = 0;
	int one, two, three, four;
	int row = 0;
	index = 0;

	int stride = width+1;

	for(int y=0;y<height;++y)
	{
		for(int x=0;x<width;++x)
		{
			one = x + row;
			two = x+row+1;
			three = x+row+stride;
			four = x+row+stride+1;

			// calculate face normal, add to normals, augment normalCount for subsequent averaging
			VuoPoint4d faceNormal = VuoMeshUtility_faceNormal(positions[one], positions[two], positions[three]);

			normals[one] 	= VuoPoint4d_add(normals[one], faceNormal);
			normals[two] 	= VuoPoint4d_add(normals[two], faceNormal);
			normals[three] 	= VuoPoint4d_add(normals[three], faceNormal);
			normals[four] 	= VuoPoint4d_add(normals[four], faceNormal);

			normalCount[one]++;
			normalCount[two]++;
			normalCount[three]++;
			normalCount[four]++;

			if(closeU && x == width-1)
			{
				// Add the first face in row normal to right-most vertices
				VuoPoint4d uNrm = VuoMeshUtility_faceNormal( positions[row], positions[row+1], positions[row+stride] );

				normals[two] 		= VuoPoint4d_add(normals[two], uNrm);
				normals[four] 		= VuoPoint4d_add(normals[four], uNrm);
				// And add the current face normal to the origin row vertex normals
				normals[row] 		= VuoPoint4d_add(normals[row], faceNormal);
				normals[row+stride] = VuoPoint4d_add(normals[row+stride], faceNormal);

				normalCount[two]++;
				normalCount[four]++;
				normalCount[row]++;
				normalCount[row+stride]++;
			}

			if(closeV && y==height-1)
			{
				VuoPoint4d vNrm = VuoMeshUtility_faceNormal( positions[x], positions[x+1], positions[x+stride] );

				normals[three] 	= VuoPoint4d_add(normals[three], vNrm);
				normals[four] 	= VuoPoint4d_add(normals[four], vNrm);
				normals[x] 		= VuoPoint4d_add(normals[x], faceNormal);
				normals[x+1] 	= VuoPoint4d_add(normals[x+1], faceNormal);

				normalCount[three]++;
				normalCount[four]++;
				normalCount[x]++;
				normalCount[x+1]++;
			}

			// Elements are wound to be front-facing for Vuo's right-handed coordinate system.
			// Order the elements so that the diagonal edge of each triangle
			// is last, so that vuo.shader.make.wireframe can optionally omit them.
			triangles[index+0] = three;
			triangles[index+1] = one;
			triangles[index+2] = two;
			triangles[index+3] = two;
			triangles[index+4] = four;
			triangles[index+5] = three;

			index += 6;
		}
		row += stride;
	}

	// average normals
	for(int i = 0; i < vertexCount; i++)
	{
		normals[i] = VuoPoint4d_multiply(normals[i], 1./(double)normalCount[i]);
	}
	free(normalCount);

	VuoSubmesh submesh;

	submesh.vertexCount = vertexCount;

	submesh.positions = positions;
	submesh.normals = normals;
	submesh.tangents = NULL;
	submesh.bitangents = NULL;
	submesh.textureCoordinates = textures;
	submesh.elementCount = triangleCount;
	submesh.elements = triangles;
	submesh.elementAssemblyMethod = VuoMesh_IndividualTriangles;
	submesh.faceCullingMode = GL_BACK;

	VuoMeshUtility_calculateTangents(&submesh);

	return VuoMesh_makeFromSingleSubmesh(submesh);
}
