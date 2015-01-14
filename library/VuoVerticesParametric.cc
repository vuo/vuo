/**
 * @file
 * VuoVerticesParametric implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include "VuoVerticesParametric.h"
#include "muParser.h"


extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoVerticesParametric",
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


static inline VuoPoint4d VuoVerticesParametric_faceNormal(VuoPoint4d a, VuoPoint4d b, VuoPoint4d c);

void VuoVerticesParametric_calculateTangentArray(int vertexCount, const VuoPoint4d *vertex, const VuoPoint4d *normal,
						   const VuoPoint4d *texcoord, int triangleCount, const int *triangle, VuoPoint4d *tangent, VuoPoint4d *bitangent);


/**
 * Generates a mesh (@c VuoVertices) given a set of mathematical expressions specifying a warped surface.
 */
VuoList_VuoVertices VuoVerticesParametric_generate( VuoText xExp, VuoText yExp, VuoText zExp, VuoText uExp, VuoText vExp, VuoInteger uSubdivisions, VuoInteger vSubdivisions, bool closeU, bool closeV )
{
	mu::Parser xParser, yParser, zParser, uParser, vParser;

	xParser.SetExpr(xExp);
	yParser.SetExpr(yExp);
	zParser.SetExpr(zExp);

	uParser.SetExpr(uExp);
	vParser.SetExpr(vExp);

	mu::value_type uVar = 0;
	mu::value_type vVar = 0;

	xParser.DefineVar("u", &uVar);
	yParser.DefineVar("u", &uVar);
	zParser.DefineVar("u", &uVar);
	uParser.DefineVar("u", &uVar);
	vParser.DefineVar("u", &uVar);
	xParser.DefineVar("v", &vVar);
	yParser.DefineVar("v", &vVar);
	zParser.DefineVar("v", &vVar);
	uParser.DefineVar("v", &vVar);
	vParser.DefineVar("v", &vVar);

	xParser.DefineConst("DEG2RAD", (double)DEG2RAD);
	yParser.DefineConst("DEG2RAD", (double)DEG2RAD);
	zParser.DefineConst("DEG2RAD", (double)DEG2RAD);
	uParser.DefineConst("DEG2RAD", (double)DEG2RAD);
	vParser.DefineConst("DEG2RAD", (double)DEG2RAD);

	xParser.DefineConst("RAD2DEG", (double)RAD2DEG);
	yParser.DefineConst("RAD2DEG", (double)RAD2DEG);
	zParser.DefineConst("RAD2DEG", (double)RAD2DEG);
	uParser.DefineConst("RAD2DEG", (double)RAD2DEG);
	vParser.DefineConst("RAD2DEG", (double)RAD2DEG);

	xParser.DefineConst("PI", (double)PI);
	yParser.DefineConst("PI", (double)PI);
	zParser.DefineConst("PI", (double)PI);
	uParser.DefineConst("PI", (double)PI);
	vParser.DefineConst("PI", (double)PI);

	int width = uSubdivisions;
	int height = vSubdivisions;

	float u = 0., v = 0., ustep = 1./(width-1.), vstep = 1./(height-1.);

	if(closeU) width--;
	if(closeV) height--;

	int vertexCount = width * height;

	VuoPoint4d *positions = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*vertexCount);
	VuoPoint4d *normals = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*vertexCount);
	VuoPoint4d *tangents = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*vertexCount);
	VuoPoint4d *bitangents = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*vertexCount);
	VuoPoint4d *textures = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*vertexCount);

	try
	{
		int i = 0;
		for(int y = 0; y < height; y++)
		{
			vVar = v;
			for(int x = 0; x < width; x++)
			{
				uVar = u;
				positions[i].x = xParser.Eval();
				positions[i].y = yParser.Eval();
				positions[i].z = zParser.Eval();
				positions[i].w = 1.;

				normals[i].x = 0.;
				normals[i].y = 0.;
				normals[i].z = 0.;
				normals[i].w = 0.;

				textures[i].x = uParser.Eval();
				textures[i].y = vParser.Eval();
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
		fprintf(stderr, "VuoVerticesParametric_generate() Error: %s\n", e.GetMsg().c_str());
		return VuoListCreate_VuoVertices();
	}

	// wind triangles

	width = uSubdivisions-1;
	height = vSubdivisions-1;

	unsigned int triangleCount = (width*height)*3*2;
	unsigned int *triangles = (unsigned int *)malloc(sizeof(unsigned int)*triangleCount);

	// Prepare an array to count how many neighboring face normals have been added to the vertex normal, so we can divide later to get the average.
	int normalCount[vertexCount];
	for (int i=0;i<vertexCount;++i)
		normalCount[i] = 0;

	int index = 0;
	int one, two, three, four;
	int row = 0;
	index = 0;

	int stride = closeU ? width : width+1;

	for(int y=0;y<height;++y)
	{
		for(int x=0;x<width;++x)
		{
			if(closeU && closeV && x==width-1 && y==height-1)
			{
				one = x + row;
				two = row;
				three = x;
				four = 0;
			}
			else
			if(x == width-1 && closeU) // close x
			{
				one = x + row;
				two = row;
				three = x+row+stride;
				four = row + stride;
			}
			else if(y == height-1 && closeV) // close y
			{
				one = x+row;
				two = x+row+1;
				three = x;
				four = x+1;
			}
			else // normal
			{
				one = x + row;
				two = x+row+1;
				three = x+row+stride;
				four = x+row+stride+1;
			}

			// calculate face normal, add to normals, augment normalCount for subsequent averaging
			VuoPoint4d faceNormal = VuoVerticesParametric_faceNormal(positions[one], positions[two], positions[three]);

			normals[one] 	= VuoPoint4d_add(normals[one], faceNormal);
			normals[two] 	= VuoPoint4d_add(normals[two], faceNormal);
			normals[three] 	= VuoPoint4d_add(normals[three], faceNormal);
			normals[four] 	= VuoPoint4d_add(normals[four], faceNormal);

			normalCount[one]++;
			normalCount[two]++;
			normalCount[three]++;
			normalCount[four]++;

			// if right handed
			triangles[index+0] = one;
			triangles[index+1] = two;
			triangles[index+2] = three;
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
		normals[i] = VuoPoint4d_divide(normals[i], (double)normalCount[i]);
//		tangents[i] = normals[i];
//		bitangents[i] = normals[i];
	}

	VuoVerticesParametric_calculateTangentArray(vertexCount, positions, normals, textures, triangleCount, (const int*)triangles, tangents, bitangents);

	VuoVertices vertices;

	vertices.vertexCount = vertexCount;

	vertices.positions = positions;
	vertices.normals = normals;
	vertices.tangents = tangents;
	vertices.bitangents = bitangents;
	vertices.textureCoordinates = textures;
	vertices.elementCount = triangleCount;
	vertices.elements = triangles;
	vertices.elementAssemblyMethod = VuoVertices_IndividualTriangles;

	VuoRegister(vertices.positions, free);
	VuoRegister(vertices.normals, free);
	VuoRegister(vertices.tangents, free);
	VuoRegister(vertices.bitangents, free);
	VuoRegister(vertices.textureCoordinates, free);
	VuoRegister(vertices.elements, free);

	VuoList_VuoVertices verticesList = VuoListCreate_VuoVertices();
	VuoListAppendValue_VuoVertices(verticesList, vertices);
	return verticesList;
}

/**
 * Calculates the face normal for position vertices @c a, @c b, and @c c.
 */
static inline VuoPoint4d VuoVerticesParametric_faceNormal(VuoPoint4d a, VuoPoint4d b, VuoPoint4d c)
{
	return VuoPoint4d_normalize3d(VuoPoint4d_crossProduct(VuoPoint4d_subtract(b,a), VuoPoint4d_subtract(c,a)));
}

/**
 * Calculates tangents and bitangents for a mesh given vertices, textures, normals, and triangles.
 * Assumes triangles are wound using VuoVertices_IndividualTriangles.
 * Lengyel, Eric. “Computing Tangent Space Basis Vectors for an Arbitrary Mesh”. Terathon Software 3D Graphics Library, 2001. http://www.terathon.com/code/tangent.html
 */
void VuoVerticesParametric_calculateTangentArray(int vertexCount, const VuoPoint4d *vertex, const VuoPoint4d *normal,
						   const VuoPoint4d *texcoord, int triangleCount, const int *triangle, VuoPoint4d *tangent, VuoPoint4d *bitangent)
{
	int tan1count = vertexCount * 2;
	VuoPoint4d *tan1 = new VuoPoint4d[tan1count];
	VuoPoint4d *tan2 = tan1 + vertexCount;

	for( int i = 0; i < tan1count; i++)
		tan1[i] = (VuoPoint4d) {0,0,0,0};

	for (int a = 0; a < triangleCount; a+=3)
	{
		long i1 = triangle[a+0];
		long i2 = triangle[a+1];
		long i3 = triangle[a+2];

		const VuoPoint4d &v1 = vertex[i1];
		const VuoPoint4d &v2 = vertex[i2];
		const VuoPoint4d &v3 = vertex[i3];

		const VuoPoint4d &w1 = texcoord[i1];
		const VuoPoint4d &w2 = texcoord[i2];
		const VuoPoint4d &w3 = texcoord[i3];

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.x - w1.x;
		float s2 = w3.x - w1.x;
		float t1 = w2.y - w1.y;
		float t2 = w3.y - w1.y;

		float r = 1.0 / (s1 * t2 - s2 * t1);
		VuoPoint4d sdir = (VuoPoint4d)
		{
			(t2 * x1 - t1 * x2) * r,
			(t2 * y1 - t1 * y2) * r,
			(t2 * z1 - t1 * z2) * r,
			1.
		};

		VuoPoint4d tdir = (VuoPoint4d)
		{
			(s1 * x2 - s2 * x1) * r,
			(s1 * y2 - s2 * y1) * r,
			(s1 * z2 - s2 * z1) * r,
			1.
		};

		tan1[i1] = VuoPoint4d_add(tan1[i1], sdir);
		tan1[i2] = VuoPoint4d_add(tan1[i2], sdir);
		tan1[i3] = VuoPoint4d_add(tan1[i3], sdir);

		tan2[i1] = VuoPoint4d_add(tan2[i1], tdir);
		tan2[i2] = VuoPoint4d_add(tan2[i2], tdir);
		tan2[i3] = VuoPoint4d_add(tan2[i3], tdir);
	}

	for (long a = 0; a < vertexCount; a++)
	{
		const VuoPoint4d& n = normal[a];
		const VuoPoint4d& t = tan1[a];
		const VuoPoint4d& t2 = tan2[a];

		// Gram-Schmidt orthogonalize
		tangent[a] = VuoPoint4d_normalize3d( VuoPoint4d_multiply( VuoPoint4d_subtract(t, n), VuoPoint4d_dotProduct(n, t)));
		bitangent[a] = VuoPoint4d_normalize3d( VuoPoint4d_multiply( VuoPoint4d_subtract(t2, n), VuoPoint4d_dotProduct(n, t2)));

		// Calculate handedness
		tangent[a].w = (VuoPoint4d_dotProduct(VuoPoint4d_crossProduct(n, t), tan2[a]) < 0.0F) ? -1.0F : 1.0F;
		bitangent[a].w = (VuoPoint4d_dotProduct(VuoPoint4d_crossProduct(n, t), tan1[a]) < 0.0F) ? -1.0F : 1.0F;
	}

	delete[] tan1;
}
