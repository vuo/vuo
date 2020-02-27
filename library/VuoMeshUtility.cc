/**
 * @file
 * VuoMeshUtility implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMeshUtility.h"
#include "module.h"
#include <vector>

extern "C"
{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "VuoMeshUtility",
					"dependencies" : [
						"VuoList_VuoPoint4d",
						"VuoList_VuoInteger"
					 ]
				 });
#endif
}

/// Constant providing the ratio of a circle's circumference to its diameter
#define PI 3.14159265359f

/**
 * Calculates the normal for each vertex in this mesh.  Vertices pointed at by multiple
 * element indices will be averaged.
 */
void VuoMeshUtility_calculateNormals(VuoMesh mesh)
{
	if (VuoMesh_getElementAssemblyMethod(mesh) != VuoMesh_IndividualTriangles)
	{
		VUserLog("VuoMeshUtility_calculateNormals() requires element assembly method VuoMesh_IndividualTriangles.");
		return;
	}

	unsigned int vertexCount, elementCount, *elements;
	float *vertices, *textureCoordinates, *colors;
	VuoMesh_getCPUBuffers(mesh, &vertexCount, &vertices, nullptr, &textureCoordinates, &colors, &elementCount, &elements);
	if (!vertexCount)
		return;

	float *normals = (float *)malloc(sizeof(float) * 3 * vertexCount);
	unsigned int* normalsAverage = (unsigned int*)malloc(sizeof(unsigned int) * vertexCount);

	// initialize normals to {0,0,0,0}
	for(unsigned int i = 0; i < vertexCount; i++)
	{
		normals[i * 3    ] = 0;
		normals[i * 3 + 1] = 0;
		normals[i * 3 + 2] = 0;
		normalsAverage[i] = 0;
	}

	// sum up all normals
	if (elementCount)
		for (unsigned int i = 0; i < elementCount; i += 3)
		{
			unsigned int i0 = elements[i + 0];
			unsigned int i1 = elements[i + 1];
			unsigned int i2 = elements[i + 2];

			VuoPoint3d a = VuoPoint3d_makeFromArray(&vertices[i0 * 3]);
			VuoPoint3d b = VuoPoint3d_makeFromArray(&vertices[i1 * 3]);
			VuoPoint3d c = VuoPoint3d_makeFromArray(&vertices[i2 * 3]);

			VuoPoint3d cross = VuoPoint3d_normalize(VuoPoint3d_crossProduct(b - a, c - a));

			VuoPoint3d n = VuoPoint3d_makeFromArray(&normals[i0 * 3]) + cross;
			VuoPoint3d_setArray(&normals[i0 * 3], n);

			n = VuoPoint3d_makeFromArray(&normals[i1 * 3]) + cross;
			VuoPoint3d_setArray(&normals[i1 * 3], n);

			n = VuoPoint3d_makeFromArray(&normals[i2 * 3]) + cross;
			VuoPoint3d_setArray(&normals[i2 * 3], n);

			normalsAverage[i0]++;
			normalsAverage[i1]++;
			normalsAverage[i2]++;
		}
	else
		for (unsigned int i = 0; i < vertexCount; i += 3)
		{
			unsigned int i0 = i + 0;
			unsigned int i1 = i + 1;
			unsigned int i2 = i + 2;

			VuoPoint3d a = VuoPoint3d_makeFromArray(&vertices[i0 * 3]);
			VuoPoint3d b = VuoPoint3d_makeFromArray(&vertices[i1 * 3]);
			VuoPoint3d c = VuoPoint3d_makeFromArray(&vertices[i2 * 3]);

			VuoPoint3d cross = VuoPoint3d_normalize(VuoPoint3d_crossProduct(b - a, c - a));

			VuoPoint3d n = VuoPoint3d_makeFromArray(&normals[i0 * 3]) + cross;
			VuoPoint3d_setArray(&normals[i0 * 3], n);

			n = VuoPoint3d_makeFromArray(&normals[i1 * 3]) + cross;
			VuoPoint3d_setArray(&normals[i1 * 3], n);

			n = VuoPoint3d_makeFromArray(&normals[i2 * 3]) + cross;
			VuoPoint3d_setArray(&normals[i2 * 3], n);

			normalsAverage[i0]++;
			normalsAverage[i1]++;
			normalsAverage[i2]++;
		}

	// now go through and average
	for (unsigned int i = 0; i < vertexCount; i++)
	{
		normals[i * 3    ] /= normalsAverage[i];
		normals[i * 3 + 1] /= normalsAverage[i];
		normals[i * 3 + 2] /= normalsAverage[i];
	}

	free(normalsAverage);

	VuoMesh_setCPUBuffers(mesh, vertexCount, vertices, normals, textureCoordinates, colors, elementCount, elements);
}

/**
 * Get the bounds of a mesh.
 */
bool VuoMeshUtility_bounds(const VuoMesh mesh, VuoPoint3d *min, VuoPoint3d *max)
{
	unsigned int vertexCount;
	float *positions;
	VuoMesh_getCPUBuffers(mesh, &vertexCount, &positions, nullptr, nullptr, nullptr, nullptr, nullptr);
	if (!vertexCount)
		return false;

	// Calculate the center of the mesh.
	*min = *max = VuoPoint3d_makeFromArray(&positions[0]);

	for (unsigned int i = 1; i < vertexCount; i++)
	{
		*min = VuoPoint3d_min(*min, VuoPoint3d_makeFromArray(&positions[i * 3]));
		*max = VuoPoint3d_max(*max, VuoPoint3d_makeFromArray(&positions[i * 3]));
	}

	return true;
}

/**
 * Generate UVs by calculating each vertex position as projected onto a bounding sphere.
 * https://en.wikipedia.org/wiki/UV_mapping
 */
void VuoMeshUtility_calculateSphericalUVs(VuoMesh mesh)
{
	if (VuoMesh_getElementAssemblyMethod(mesh) == VuoMesh_IndividualTriangles)
		VuoMeshUtility_insertSeam(mesh);

	unsigned int vertexCount, elementCount, *elements;
	float *vertices, *normals, *colors;
	VuoMesh_getCPUBuffers(mesh, &vertexCount, &vertices, &normals, nullptr, &colors, &elementCount, &elements);
	if (!vertexCount)
		return;

	unsigned int dup_vertex_start = vertexCount;

	float *textureCoordinates = (float *)malloc(sizeof(float) * 2 * vertexCount);

	VuoPoint3d center;
	{
		VuoPoint3d min, max;
		VuoMeshUtility_bounds(mesh, &min, &max);
		center = (min + max) / 2.f;
	}

	VuoPoint2d min, max;
	for (unsigned int i = 0; i < vertexCount; i++)
	{
		VuoPoint3d v = VuoPoint3d_normalize(vertices[i] - center);

		VuoPoint2d tc = (VuoPoint2d){
			1.f - (.5f + (atan2f(v.z, v.x) / (2.f * PI))),
			1.f - (.5f - (asinf(v.y) / PI))};

		if(i >= dup_vertex_start)
			tc.x -= 1.;

		if(i == 0)
		{
			min = tc;
			max = tc;
		}

		min.x = fmin(min.x, tc.x);
		min.y = fmin(min.y, tc.y);

		max.x = fmax(max.x, tc.x);
		max.y = fmax(max.y, tc.y);

		textureCoordinates[i * 2    ] = tc.x;
		textureCoordinates[i * 2 + 1] = tc.y;
	}

	VuoPoint2d scale = { 1 / (max.x - min.x), 1 / (max.y - min.y) };

	for (unsigned int i = 0; i < vertexCount; i++)
	{
		textureCoordinates[i * 2    ] -= min.x;
		textureCoordinates[i * 2 + 1] -= min.y;
		textureCoordinates[i * 2    ] *= scale.x;
		textureCoordinates[i * 2 + 1] *= scale.y;
	}

	VuoMesh_setCPUBuffers(mesh, vertexCount, vertices, normals, textureCoordinates, colors, elementCount, elements);

	// remove vertices *after* generating textures because
	// textures uses the overflowing vertices to determine
	// which coordinates need to be wrapped.
	if (VuoMesh_getElementAssemblyMethod(mesh) == VuoMesh_IndividualTriangles && dup_vertex_start < vertexCount)
		VuoMeshUtility_removeUnusedVertices(mesh);
}

/**
 * Defines a direction on the X, Y, or Z axis (and their negated direction).
 */
enum Plane {
	PlaneX,
	PlaneY,
	PlaneZ,
	PlaneNegX,
	PlaneNegY,
	PlaneNegZ
};

/**
 * Returns the plane with the axis most similar to `normal`.
 * Ex: { .2, .1, .9 } returns PlaneZ.
 */
Plane VuoMeshUtility_calculateBestPlane(VuoPoint3d normal)
{
	float 	x = fabs(normal.x),
			y = fabs(normal.y),
			z = fabs(normal.z);

	if( x > y && x > z ) {
		return normal.x < 0 ? PlaneNegX : PlaneX;
	} else if( y > x && y > z ) {
		return normal.y < 0 ? PlaneNegY : PlaneY;
	} else {
		return normal.z < 0 ? PlaneNegZ : PlaneZ;
	}
}

/**
 * Averages the x, y, z components of points `a`, `b`, `c`.
 */
static inline VuoPoint3d VuoMeshUtility_averageNormal(VuoPoint3d a, VuoPoint3d b, VuoPoint3d c)
{
	return (VuoPoint3d){(a.x + b.x + c.x) / 3.f,
						(a.y + b.y + c.y) / 3.f,
						(a.z + b.z + c.z) / 3.f};
}

/**
 * Generate cubic UVs for this mesh.  Works best with IndividualTriangles meshes where no vertex is shared between triangles.
 */
void VuoMeshUtility_calculateCubicUVs(VuoMesh mesh)
{
	VuoPoint3d min, max;
	VuoMeshUtility_bounds(mesh, &min, &max);
	VuoPoint3d range = max - min;

	unsigned int vertexCount, elementCount, *elements;
	float *positions, *normals, *colors;
	VuoMesh_getCPUBuffers(mesh, &vertexCount, &positions, &normals, nullptr, &colors, &elementCount, &elements);
	if (!vertexCount || !normals)
		return;

	float *textureCoordinates = (float *)malloc(sizeof(float) * 2 * vertexCount);

	for (unsigned int i = 0; i < vertexCount; i++)
	{
		VuoPoint3d p = VuoPoint3d_makeFromArray(&positions[i * 3]);
		VuoPoint3d n = VuoPoint3d_makeFromArray(&normals[i * 3]);
		VuoPoint3d uv = p - min;

		uv.x = uv.x / range.x;
		uv.y = uv.y / range.y;
		uv.z = uv.z / range.z;

		switch (VuoMeshUtility_calculateBestPlane(n))
		{
			case PlaneX:
			case PlaneNegX:
				textureCoordinates[i * 2    ] = uv.z;
				textureCoordinates[i * 2 + 1] = uv.y;
				break;

			case PlaneY:
			case PlaneNegY:
				textureCoordinates[i * 2    ] = uv.x;
				textureCoordinates[i * 2 + 1] = uv.z;
				break;

			case PlaneZ:
			case PlaneNegZ:
				textureCoordinates[i * 2    ] = uv.x;
				textureCoordinates[i * 2 + 1] = uv.y;
				break;
		}
	}

	VuoMesh_setCPUBuffers(mesh, vertexCount, positions, normals, textureCoordinates, colors, elementCount, elements);
}

/**
 * Generate cubic UVs for this mesh, using the triangle normal to project UV instead of vertex normal.
 */
void VuoMeshUtility_calculateCubicUVsPerTriangle(VuoMesh mesh)
{
	VuoPoint3d min, max;
	VuoMeshUtility_bounds(mesh, &min, &max);
	VuoPoint3d range = max - min;

	unsigned int vertexCount, elementCount, *elements;
	float *positions, *normals, *colors;
	VuoMesh_getCPUBuffers(mesh, &vertexCount, &positions, &normals, nullptr, &colors, &elementCount, &elements);
	if (!vertexCount || !normals || !elementCount)
		return;

	float *textureCoordinates = (float *)malloc(sizeof(float) * 2 * vertexCount);

	for (unsigned int i = 0; i < elementCount; i+=3)
	{
		unsigned int    a = elements[i+0],
						b = elements[i+1],
						c = elements[i+2];

		VuoPoint3d pa = VuoPoint3d_makeFromArray(&positions[a * 3]);
		VuoPoint3d pb = VuoPoint3d_makeFromArray(&positions[b * 3]);
		VuoPoint3d pc = VuoPoint3d_makeFromArray(&positions[c * 3]);

		VuoPoint3d uv_a = pa - min;
		VuoPoint3d uv_b = pb - min;
		VuoPoint3d uv_c = pc - min;

		uv_a.x = uv_a.x / range.x;
		uv_b.x = uv_b.x / range.x;
		uv_c.x = uv_c.x / range.x;
		uv_a.y = uv_a.y / range.y;
		uv_b.y = uv_b.y / range.y;
		uv_c.y = uv_c.y / range.y;
		uv_a.z = uv_a.z / range.z;
		uv_b.z = uv_b.z / range.z;
		uv_c.z = uv_c.z / range.z;

		switch (VuoMeshUtility_calculateBestPlane( VuoMeshUtility_averageNormal(normals[a], normals[b], normals[c])))
		{
			case PlaneX:
			case PlaneNegX:
				textureCoordinates[a * 2    ] = uv_a.z;
				textureCoordinates[a * 2 + 1] = uv_a.y;
				textureCoordinates[b * 2    ] = uv_b.z;
				textureCoordinates[b * 2 + 1] = uv_b.y;
				textureCoordinates[c * 2    ] = uv_c.z;
				textureCoordinates[c * 2 + 1] = uv_c.y;
				break;

			case PlaneY:
			case PlaneNegY:
				textureCoordinates[a * 2    ] = uv_a.x;
				textureCoordinates[a * 2 + 1] = uv_a.z;
				textureCoordinates[b * 2    ] = uv_b.x;
				textureCoordinates[b * 2 + 1] = uv_b.z;
				textureCoordinates[c * 2    ] = uv_c.x;
				textureCoordinates[c * 2 + 1] = uv_c.z;
				break;

			case PlaneZ:
			case PlaneNegZ:
				textureCoordinates[a * 2    ] = uv_a.x;
				textureCoordinates[a * 2 + 1] = uv_a.y;
				textureCoordinates[b * 2    ] = uv_b.x;
				textureCoordinates[b * 2 + 1] = uv_b.y;
				textureCoordinates[c * 2    ] = uv_c.x;
				textureCoordinates[c * 2 + 1] = uv_c.y;
				break;
		}
	}

	VuoMesh_setCPUBuffers(mesh, vertexCount, positions, normals, textureCoordinates, colors, elementCount, elements);
}

/**
 * Inserts a seam on a mesh.  May (probably will) leave unused vertices.
 */
void VuoMeshUtility_insertSeam(VuoMesh mesh)
{
	unsigned int vertexCount, elementCount, *indices;
	float *positions, *normals, *textures;
	VuoMesh_getCPUBuffers(mesh, &vertexCount, &positions, &normals, &textures, nullptr, &elementCount, &indices);
	if (!vertexCount || !elementCount)
		return;

	unsigned int *seam_indices = (unsigned int *)malloc(sizeof(unsigned int) * elementCount);
	memcpy(seam_indices, indices, sizeof(unsigned int) * elementCount);

	std::vector<float> seam_positions = std::vector<float>(positions != NULL ? vertexCount * 3 : 0);
	if(positions) std::copy(positions, positions + vertexCount * 3, seam_positions.begin());
	std::vector<float> seam_normals   = std::vector<float>(normals != NULL ? vertexCount * 3 : 0);
	if(normals) std::copy(normals, normals + vertexCount * 3, seam_normals.begin());
	std::vector<float> seam_textures  = std::vector<float>(textures != NULL ? vertexCount * 2 : 0);
	if(textures) std::copy(textures, textures + vertexCount * 2, seam_textures.begin());

	const VuoPoint3d left    = (VuoPoint3d){-1.,  0., 0.};
	const VuoPoint3d forward = (VuoPoint3d){ 0.,  0., 1.};
	const VuoPoint3d up      = (VuoPoint3d){ 0.,  1., 0.};
	const VuoPoint3d down    = (VuoPoint3d){ 0., -1., 0.};

	// get mesh bounds and center point
	VuoPoint3d min, max;
	VuoMeshUtility_bounds(mesh, &min, &max);
	VuoPoint3d center = (max + min) / 2.f;

	// Insert vertical seam along western hemisphere
	for (unsigned int i = 0; i < elementCount; i+=3)
	{
		if(indices[i] > vertexCount || indices[i+1] > vertexCount || indices[i+2] > vertexCount)
		{
			VUserLog("Triangle indices are referencing out of bounds vertices. (%u, %u, %u)  Vertex Count: %u", indices[i], indices[i+1], indices[i+2], vertexCount);
			continue;
		}
		// {0,1} {1,2}, {2,0}
		for(int n = 0; n < 3; n++)
		{
			unsigned int x = i + n;
			unsigned int y = i + (n == 2 ? 0 : n+1);

			// check if this triangle edge crosses the new seam
			VuoPoint3d px = VuoPoint3d_makeFromArray(&positions[indices[x] * 3]);
			VuoPoint3d py = VuoPoint3d_makeFromArray(&positions[indices[y] * 3]);
			VuoPoint3d v0 = VuoPoint3d_normalize(px - center);
			VuoPoint3d v1 = VuoPoint3d_normalize(py - center);

			if (VuoPoint3d_areEqual(v0, up) || VuoPoint3d_areEqual(v0, down)
			 || VuoPoint3d_areEqual(v1, up) || VuoPoint3d_areEqual(v1, down))
				continue;

			// if v0 is in left back quadrant, and v1 is in the forward half, v0 needs to be duplicated.  Ditto in opposite order
			if (VuoPoint3d_dotProduct(v0, left) > 0
			 && VuoPoint3d_dotProduct(v0, forward) < 0
			 && VuoPoint3d_dotProduct(v1, forward) >= 0)
			{
				if (positions)
				{
					seam_positions.push_back(positions[indices[x] * 3    ]);
					seam_positions.push_back(positions[indices[x] * 3 + 1]);
					seam_positions.push_back(positions[indices[x] * 3 + 2]);
				}
				if (normals)
				{
					seam_normals.push_back(normals[indices[x] * 3    ]);
					seam_normals.push_back(normals[indices[x] * 3 + 1]);
					seam_normals.push_back(normals[indices[x] * 3 + 2]);
				}
				if (textures)
				{
					seam_textures.push_back(textures[indices[x] * 2    ]);
					seam_textures.push_back(textures[indices[x] * 2 + 1]);
				}

				seam_indices[x] = seam_positions.size() / 3 - 1;
			}

			// if v1 is in left back quadrant, but v0 isn't, duplicate v1
			if (VuoPoint3d_dotProduct(v1, left) > 0
			 && VuoPoint3d_dotProduct(v1, forward) < 0
			 && VuoPoint3d_dotProduct(v0, forward) >= 0)
			{
				if(positions)
				{
					seam_positions.push_back(positions[indices[y] * 3    ]);
					seam_positions.push_back(positions[indices[y] * 3 + 1]);
					seam_positions.push_back(positions[indices[y] * 3 + 2]);
				}
				if(normals)
				{
					seam_normals.push_back(normals[indices[y] * 3    ]);
					seam_normals.push_back(normals[indices[y] * 3 + 1]);
					seam_normals.push_back(normals[indices[y] * 3 + 2]);
				}
				if(textures)
				{
					seam_textures.push_back(textures[indices[y] * 2    ]);
					seam_textures.push_back(textures[indices[y] * 2 + 1]);
				}

				seam_indices[y] = seam_positions.size() / 3 - 1;
			}
		}
	}

	float *newPositions = (float *)malloc(sizeof(float) * seam_positions.size());
	std::copy(seam_positions.begin(), seam_positions.end(), newPositions);

	float *newNormals = nullptr;
	if (normals)
	{
		newNormals = (float *)malloc(sizeof(float) * seam_normals.size());
		std::copy(seam_normals.begin(), seam_normals.end(), newNormals);
	}

	float *newTextureCoordinates = nullptr;
	if (textures)
	{
		newTextureCoordinates = (float *)malloc(sizeof(float) * seam_textures.size());
		std::copy(seam_textures.begin(), seam_textures.end(), newTextureCoordinates);
	}

	VuoMesh_setCPUBuffers(mesh, seam_positions.size() / 3,
		newPositions, newNormals, newTextureCoordinates, nullptr,
		elementCount, seam_indices);
}

/**
 * Compare function for triangle ordering qsort in removeUnusedVertices.
 */
static int compare(const void* lhs, const void* rhs)
{
	return ( *(int*)lhs - *(int*)rhs );
}

/**
 * Removes unused vertices from a mesh.
 */
void VuoMeshUtility_removeUnusedVertices(VuoMesh mesh)
{
	unsigned int vertexCount, elementCount, *elements;
	float *positions, *normals, *textureCoordinates;
	VuoMesh_getCPUBuffers(mesh, &vertexCount, &positions, &normals, &textureCoordinates, nullptr, &elementCount, &elements);
	if (!vertexCount || !elementCount)
		return;

	unsigned int* sortedTriangles = (unsigned int*)malloc(sizeof(unsigned int) * elementCount);
	memcpy(sortedTriangles, elements, sizeof(unsigned int) * elementCount);

	// sort indices from smallest to largest
	qsort(sortedTriangles, elementCount, sizeof(unsigned int), compare);

	// lop off any elements that are referencing out of bounds vertices
	unsigned int last = elementCount-1;

	while( sortedTriangles[last] > vertexCount )
		last--;

	// if there are invalid indices, reallocate the the elements array with only the good'ns.
	if(last < elementCount-1)
	{
		unsigned int* pruned_elements = (unsigned int*)malloc(sizeof(unsigned int) * (last+1));

		unsigned int n = 0;

		for(unsigned int i = 0; i < elementCount; i++)
		{
			if (elements[i] < vertexCount)
				pruned_elements[n++] = elements[i];
		}

		elementCount = last+1;
		elements = pruned_elements;
	}

	// generate list of unused indices
	std::vector<unsigned int> unused;

	int lastVal = -1;
	for(int i = 0; i < elementCount && sortedTriangles[i] < elementCount; i++)
	{
		for(int n = lastVal+1; n < sortedTriangles[i]; n++)
		{
			unused.push_back(n);
		}

		lastVal = sortedTriangles[i];
	}
	free(sortedTriangles);

	// rebuild index array by substracting the amount of vertices removed in the range lower
	// than element[i]
	int unusedCount = unused.size();
	for(int i = 0; i < elementCount; i++)
	{
		unsigned int vertex = elements[i];

		if(vertex > vertexCount)
			continue;

		int n = 0;

		for(auto& val : unused)
		{
			if(val < vertex)
				n++;
			else
				break;
		}

		elements[i] -= n;
	}

	// rebuild vertex arrays without the unused values
	// not using vertexCount-unusedCount because unused may contain duplicates
	std::vector<float> new_positions          = std::vector<float>(vertexCount * 3);
	std::vector<float> new_normals            = std::vector<float>(normals ? vertexCount * 3 : 0);
	std::vector<float> new_textureCoordinates = std::vector<float>(textureCoordinates ? vertexCount * 2 : 0);

	unsigned int curIndex = 0, n = 0;
	for(int i = 0; i < vertexCount; i++)
	{
		if(curIndex >= unusedCount || i < unused[curIndex])
		{
			new_positions[n * 3    ] = positions[i * 3    ];
			new_positions[n * 3 + 1] = positions[i * 3 + 1];
			new_positions[n * 3 + 2] = positions[i * 3 + 2];
			if (normals)
			{
				new_normals[n * 3    ] = normals[i * 3    ];
				new_normals[n * 3 + 1] = normals[i * 3 + 1];
				new_normals[n * 3 + 2] = normals[i * 3 + 2];
			}
			if (textureCoordinates)
			{
				new_textureCoordinates[n * 2    ] = textureCoordinates[i * 2    ];
				new_textureCoordinates[n * 2 + 1] = textureCoordinates[i * 2 + 1];
			}

			n++;
		}
		else
		{
			do {
				curIndex++;
			} while(curIndex < unusedCount && unused[curIndex] <= i);
		}
	}

	// Re-apply points to mesh
	unsigned int culledVertexCount = new_positions.size();

	float *newPositions = (float *)malloc(sizeof(float) * 3 * culledVertexCount);
	std::copy(new_positions.begin(), new_positions.end(), newPositions);

	float *newNormals = nullptr;
	if (normals)
	{
		newNormals = (float *)malloc(sizeof(float) * 3 * culledVertexCount);
		std::copy(new_normals.begin(), new_normals.end(), newNormals);
	}

	float *newTextureCoordinates = nullptr;
	if (textureCoordinates)
	{
		newTextureCoordinates = (float *)malloc(sizeof(float) * 2 * culledVertexCount);
		std::copy(new_textureCoordinates.begin(), new_textureCoordinates.end(), newTextureCoordinates);
	}

	VuoMesh_setCPUBuffers(mesh, culledVertexCount,
		newPositions, newNormals, newTextureCoordinates, nullptr,
		elementCount, elements);
}
