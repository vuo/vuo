/**
 * @file
 * vuo.scene.make.random.points node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <OpenGL/CGLMacro.h>	// GL_NONE
#include "VuoDistribution3d.h"

VuoModuleMetadata({
					"title" : "Make Random Points Object",
					"keywords" : [ ],
					"version" : "1.0.0",
					"genericTypes" : {
						"VuoGenericType1" : {
							"compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
						}
					},
					"node": {
						"exampleCompositions" : [ "ShowRandomPoints.vuo" ]
					}
				 });

void nodeEvent
(
		VuoInputData(VuoTransform) transform,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) material,
		VuoInputData(VuoDistribution3d, {"default":"cube-volume"}) distribution,
		VuoInputData(VuoInteger, {"default":256, "suggestedMin":1, "suggestedMax":65536}) count,
		VuoInputData(VuoReal, {"default":0.01, "suggestedMin":0.0, "suggestedMax":0.5}) pointSize,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":0}) seed,
		VuoOutputData(VuoSceneObject) object
)
{
	unsigned int vertexCount = MAX(1, count);

	VuoPoint4d *vertices   = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d *normals    = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d *tangents   = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d *bitangents = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d *textures   = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);

	unsigned short randomState[3];
	VuoInteger_setRandomState(randomState, seed);

	if (distribution == VuoDistribution3d_CubeVolume)
		for (unsigned int i = 0; i < vertexCount; ++i)
			vertices[i] = (VuoPoint4d){
					VuoReal_randomWithState(randomState, -.5, .5),
					VuoReal_randomWithState(randomState, -.5, .5),
					VuoReal_randomWithState(randomState, -.5, .5),
					1};
	else if (distribution == VuoDistribution3d_CubeSurface)
		for (unsigned int i = 0; i < vertexCount; ++i)
		{
			// Choose a random face…
			VuoInteger f = VuoInteger_randomWithState(randomState, 0, 5);
			// …and choose a random position on that face.
			VuoPoint2d n = VuoPoint2d_randomWithState(randomState,
													  (VuoPoint2d){-.5,-.5},
													  (VuoPoint2d){ .5, .5});
			VuoPoint3d p = {0,0,0};
			VuoInteger sign = (f&1) ? 1 : -1;
			if (f&2)
				vertices[i] = (VuoPoint4d){n.x, n.y, .5 * sign, 1};
			else if (f&4)
				vertices[i] = (VuoPoint4d){n.x, .5 * sign, n.y, 1};
			else
				vertices[i] = (VuoPoint4d){.5 * sign, n.x, n.y, 1};
		}
	else if (distribution == VuoDistribution3d_SphereVolume)
		for (unsigned int i = 0; i < vertexCount; ++i)
		{
			// Based on http://psgraphics.blogspot.com/2015/03/easy-random-point-in-sphere.html
			VuoPoint3d p;
			do
			{
				// Choose a random point in a cube…
				p = VuoPoint3d_randomWithState(randomState, (VuoPoint3d){-1,-1,-1}, (VuoPoint3d){ 1, 1, 1});

				// …and check whether it's inside the sphere.
			} while (p.x*p.x + p.y*p.y + p.z*p.z > 1);

			vertices[i] = (VuoPoint4d){p.x/2, p.y/2, p.z/2, 1};
		}
	else if (distribution == VuoDistribution3d_SphereSurface)
		for (unsigned int i = 0; i < vertexCount; ++i)
		{
			// Based on https://www.cs.cmu.edu/~mws/rpos.html
			VuoPoint2d p = VuoPoint2d_randomWithState(randomState,
													  (VuoPoint2d){-1, 0},
													  (VuoPoint2d){ 1, M_PI*2});
			float z = p.x;
			float phi = p.y;
			float theta = asin(z);
			float ct = cos(theta);
			vertices[i] = (VuoPoint4d){ct*cos(phi)/2., ct*sin(phi)/2., z/2., 1};
		}

	for (unsigned int i = 0; i < vertexCount; ++i)
	{
		normals[i]    = (VuoPoint4d){0, 0, 1, 1};
		tangents[i]   = (VuoPoint4d){1, 0, 0, 1};
		bitangents[i] = (VuoPoint4d){0, 1, 0, 1};
		textures[i]   = (VuoPoint4d){
				VuoReal_randomWithState(randomState, 0, 1),
				VuoReal_randomWithState(randomState, 0, 1),
				0, 1};
	}

	VuoSubmesh submesh;
	submesh.vertexCount = vertexCount;
	submesh.positions = vertices;
	submesh.normals = normals;
	submesh.tangents = tangents;
	submesh.bitangents = bitangents;
	submesh.textureCoordinates = textures;
	submesh.faceCullingMode = GL_NONE;
	submesh.elementCount = 0;
	submesh.primitiveSize = pointSize;
	submesh.elements = NULL;
	submesh.elementAssemblyMethod = VuoMesh_Points;

	VuoMesh mesh = VuoMesh_makeFromSingleSubmesh(submesh);

	VuoShader shader = VuoShader_make_VuoGenericType1(material);
	*object = VuoSceneObject_make(mesh, shader, transform, NULL);
}
