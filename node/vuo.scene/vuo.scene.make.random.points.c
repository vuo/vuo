/**
 * @file
 * vuo.scene.make.random.points node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <OpenGL/CGLMacro.h>	// GL_NONE
#include "VuoDistribution3d.h"

VuoModuleMetadata({
	"title": "Make Random Points Object",
	"keywords": [
		"3D", "stars", "starfield", "shape",
	],
	"version": "1.1.1",
	"genericTypes": {
		"VuoGenericType1": {
			"compatibleTypes": [ "VuoShader", "VuoColor", "VuoImage" ],
		},
	},
	"node": {
		"exampleCompositions": [ "ShowRandomPoints.vuo" ],
	},
});

struct nodeInstanceData
{
	VuoDistribution3d distribution;
	VuoInteger count;
	VuoInteger seed;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	return context;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoTransform) transform,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) material,
		VuoInputData(VuoDistribution3d, {"default":"cube-volume"}) distribution,
		VuoInputData(VuoInteger, {"default":256, "suggestedMin":1, "suggestedMax":65536}) count,
		VuoInputData(VuoReal, {"default":0.01, "suggestedMin":0.0, "suggestedMax":0.5}) pointSize,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":0}) seed,
		VuoOutputData(VuoSceneObject) object
)
{
	// If the structure hasn't changed, just reuse the existing GPU mesh data.
	if (*object
	 && distribution == (*context)->distribution
	 && count == (*context)->count
	 && seed == (*context)->seed)
	{
		*object = VuoSceneObject_copy(*object);
		VuoSceneObject_setTransform(*object, transform);
		VuoSceneObject_setShader(*object, VuoShader_make_VuoGenericType1(material));
		VuoMesh m = VuoMesh_copyShallow(VuoSceneObject_getMesh(*object));
		VuoMesh_setPrimitiveSize(m, pointSize);
		VuoSceneObject_setMesh(*object, m);
		return;
	}

	unsigned int vertexCount = MAX(1, count);

	float *positions, *normals, *textureCoordinates;
	VuoMesh_allocateCPUBuffers(vertexCount, &positions, &normals, &textureCoordinates, NULL, 0, NULL);

	unsigned short randomState[3];
	VuoInteger_setRandomState(randomState, seed);

	if (distribution == VuoDistribution3d_CubeVolume)
		for (unsigned int i = 0; i < vertexCount; ++i)
		{
			positions[i * 3    ] = VuoReal_randomWithState(randomState, -.5, .5);
			positions[i * 3 + 1] = VuoReal_randomWithState(randomState, -.5, .5);
			positions[i * 3 + 2] = VuoReal_randomWithState(randomState, -.5, .5);
		}
	else if (distribution == VuoDistribution3d_CubeSurface)
		for (unsigned int i = 0; i < vertexCount; ++i)
		{
			// Choose a random face…
			VuoInteger f = VuoInteger_randomWithState(randomState, 0, 5);
			// …and choose a random position on that face.
			VuoPoint2d n = VuoPoint2d_randomWithState(randomState,
													  (VuoPoint2d){-.5,-.5},
													  (VuoPoint2d){ .5, .5});
			VuoInteger sign = (f&1) ? 1 : -1;
			if (f&2)
			{
				positions[i * 3    ] = n.x;
				positions[i * 3 + 1] = n.y;
				positions[i * 3 + 2] = .5 * sign;
			}
			else if (f&4)
			{
				positions[i * 3    ] = n.x;
				positions[i * 3 + 1] = .5 * sign;
				positions[i * 3 + 2] = n.y;
			}
			else
			{
				positions[i * 3    ] = .5 * sign;
				positions[i * 3 + 1] = n.x;
				positions[i * 3 + 2] = n.y;
			}
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

			positions[i * 3    ] = p.x / 2;
			positions[i * 3 + 1] = p.y / 2;
			positions[i * 3 + 2] = p.z / 2;
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
			positions[i * 3    ] = ct * cos(phi) / 2.;
			positions[i * 3 + 1] = ct * sin(phi) / 2.;
			positions[i * 3 + 2] = z / 2.;
		}

	for (unsigned int i = 0; i < vertexCount; ++i)
	{
		normals[i * 3    ] = 0;
		normals[i * 3 + 1] = 0;
		normals[i * 3 + 2] = 1;
		textureCoordinates[i * 2    ] = VuoReal_randomWithState(randomState, 0, 1);
		textureCoordinates[i * 2 + 1] = VuoReal_randomWithState(randomState, 0, 1);
	}

	VuoMesh mesh = VuoMesh_makeFromCPUBuffers(vertexCount,
		positions, normals, textureCoordinates, NULL,
		0, NULL, VuoMesh_Points);
	VuoMesh_setFaceCulling(mesh, VuoMesh_CullNone);
	VuoMesh_setPrimitiveSize(mesh, pointSize);

	VuoShader shader = VuoShader_make_VuoGenericType1(material);
	*object = VuoSceneObject_makeMesh(mesh, shader, transform);

	(*context)->distribution = distribution;
	(*context)->count = count;
	(*context)->seed = seed;
}
