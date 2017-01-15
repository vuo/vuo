/**
 * @file
 * VuoMeshParametric implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include "VuoMeshParametric.h"
#include "muParser.h"
#include <OpenGL/CGLMacro.h>
#include "VuoMeshUtility.h"
#include "VuoMathExpressionParser.h"

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoMeshParametric",
					 "dependencies" : [
						 "muParser",
						 "VuoMathExpressionParser",
						 "VuoMeshUtility"
					 ]
				 });
#endif
}

/**
 * Generates a mesh given a set of mathematical expressions specifying a warped surface.
 */
VuoMesh VuoMeshParametric_generate(VuoReal time, VuoText xExp, VuoText yExp, VuoText zExp, VuoInteger uSubdivisions, VuoInteger vSubdivisions, bool closeU, VuoReal uMin, VuoReal uMax, bool closeV, VuoReal vMin, VuoReal vMax)
{
	if (uSubdivisions < 2 || vSubdivisions < 2 || !xExp || !yExp || !zExp)
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

	VuoMathExpressionParser_defineStandardLibrary(&xParser);
	VuoMathExpressionParser_defineStandardLibrary(&yParser);
	VuoMathExpressionParser_defineStandardLibrary(&zParser);

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
		VUserLog("Error: %s", e.GetMsg().c_str());
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
