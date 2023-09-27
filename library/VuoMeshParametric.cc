/**
 * @file
 * VuoMeshParametric implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMeshParametric.h"
#include <muParser/muParser.h>
#include "VuoMathExpressionParser.h"
#include "VuoMeshUtility.h"

extern "C"
{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoMeshParametric",
					 "dependencies" : [
						 "muparser",
						 "VuoMathExpressionParser",
						 "VuoMeshUtility"
					 ]
				 });
#endif
}

/**
 * Adds a normal to a running sum.
 */
static inline void add(float *normals, int index, VuoPoint3d normal)
{
	normals[index * 3    ] += normal.x;
	normals[index * 3 + 1] += normal.y;
	normals[index * 3 + 2] += normal.z;
}

/**
 * Generates a mesh given a set of mathematical expressions specifying a warped surface.
 */
VuoMesh VuoMeshParametric_generate(VuoReal time, VuoText xExp, VuoText yExp, VuoText zExp, VuoInteger uSubdivisions, VuoInteger vSubdivisions, bool closeU, VuoReal uMin, VuoReal uMax, bool closeV, VuoReal vMin, VuoReal vMax, VuoDictionary_VuoText_VuoReal *constants)
{
	if (uSubdivisions < 2 || vSubdivisions < 2 || VuoText_isEmpty(xExp) || VuoText_isEmpty(yExp) || VuoText_isEmpty(zExp))
		return nullptr;

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

	if (constants)
	{
		unsigned long constantCount = VuoListGetCount_VuoText(constants->keys);
		VuoText *constantKeys = VuoListGetData_VuoText(constants->keys);
		VuoReal *constantValues = VuoListGetData_VuoReal(constants->values);
		for (unsigned long i = 0; i < constantCount; ++i)
		{
			xParser.DefineConst(constantKeys[i], constantValues[i]);
			yParser.DefineConst(constantKeys[i], constantValues[i]);
			zParser.DefineConst(constantKeys[i], constantValues[i]);
		}
	}

	int width = uSubdivisions;
	int height = vSubdivisions;

	float ustep = 1./(width-1.), vstep = 1./(height-1.);

	int vertexCount = width * height;

	unsigned int elementCount = ((uSubdivisions-1)*(vSubdivisions-1))*3*2;
	float *positions, *normals, *textureCoordinates;
	unsigned int *elements;
	VuoMesh_allocateCPUBuffers(vertexCount, &positions, &normals, &textureCoordinates, nullptr, elementCount, &elements);
	bzero(normals, sizeof(float) * 3 * vertexCount);

	try
	{
		int i = 0;
		float u = 0., v = 0.;
		for(int y = 0; y < height; y++)
		{
			vVar = VuoReal_lerp(vMin, vMax, (closeV && y==height-1) ? 0 : v);
			for(int x = 0; x < width; x++)
			{
				uVar = VuoReal_lerp(uMin, uMax, (closeU && x==width-1) ? 0 : u);

				iVar = x + 1;
				jVar = y + 1;

				positions[i * 3    ] = xParser.Eval();
				positions[i * 3 + 1] = yParser.Eval();
				positions[i * 3 + 2] = zParser.Eval();

				textureCoordinates[i * 2    ] = u;
				textureCoordinates[i * 2 + 1] = v;

				u += ustep;
				i++;
			}

			u = 0.;
			v += vstep;
		}
	}
	catch (mu::Parser::exception_type &e)
	{
		VUserLog("Error: %s", e.GetMsg().c_str());
		free(positions);
		free(normals);
		free(textureCoordinates);
		return nullptr;
	}

	// wind triangles

	width = uSubdivisions-1;
	height = vSubdivisions-1;

	// Prepare an array to count how many neighboring face normals have been added to the vertex normal, so we can divide later to get the average.
	unsigned int* normalCount = (unsigned int*)calloc(sizeof(unsigned int) * vertexCount, sizeof(unsigned int));

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
			VuoPoint3d faceNormal = VuoMeshUtility_faceNormal(
				VuoPoint3d_makeFromArray(&positions[one   * 3]),
				VuoPoint3d_makeFromArray(&positions[two   * 3]),
				VuoPoint3d_makeFromArray(&positions[three * 3]));

			add(normals, one, faceNormal);
			add(normals, two, faceNormal);
			add(normals, three, faceNormal);
			add(normals, four, faceNormal);

			normalCount[one]++;
			normalCount[two]++;
			normalCount[three]++;
			normalCount[four]++;

			if(closeU && x == width-1)
			{
				// Add the first face in row normal to right-most vertices
				VuoPoint3d uNrm = VuoMeshUtility_faceNormal(
					VuoPoint3d_makeFromArray(&positions[row            * 3]),
					VuoPoint3d_makeFromArray(&positions[(row + 1)      * 3]),
					VuoPoint3d_makeFromArray(&positions[(row + stride) * 3]));

				add(normals, two,          uNrm);
				add(normals, four,         uNrm);
				// And add the current face normal to the origin row vertex normals
				add(normals, row,          faceNormal);
				add(normals, row + stride, faceNormal);

				normalCount[two]++;
				normalCount[four]++;
				normalCount[row]++;
				normalCount[row+stride]++;
			}

			if(closeV && y==height-1)
			{
				VuoPoint3d vNrm = VuoMeshUtility_faceNormal(
					VuoPoint3d_makeFromArray(&positions[x            * 3]),
					VuoPoint3d_makeFromArray(&positions[(x + 1)      * 3]),
					VuoPoint3d_makeFromArray(&positions[(x + stride) * 3]));

				add(normals, three, vNrm);
				add(normals, four,  vNrm);
				add(normals, x,     faceNormal);
				add(normals, x + 1, faceNormal);

				normalCount[three]++;
				normalCount[four]++;
				normalCount[x]++;
				normalCount[x+1]++;
			}

			// Elements are wound to be front-facing for Vuo's right-handed coordinate system.
			// Order the elements so that the diagonal edge of each triangle
			// is last, so that vuo.shader.make.wireframe can optionally omit them.
			elements[index    ] = three;
			elements[index + 1] = one;
			elements[index + 2] = two;
			elements[index + 3] = two;
			elements[index + 4] = four;
			elements[index + 5] = three;

			index += 6;
		}
		row += stride;
	}

	// average normals
	for(int i = 0; i < vertexCount; i++)
	{
		normals[i * 3    ] /= normalCount[i];
		normals[i * 3 + 1] /= normalCount[i];
		normals[i * 3 + 2] /= normalCount[i];
	}
	free(normalCount);

	return VuoMesh_makeFromCPUBuffers(vertexCount,
		positions, normals, textureCoordinates, nullptr,
		elementCount, elements, VuoMesh_IndividualTriangles);
}
