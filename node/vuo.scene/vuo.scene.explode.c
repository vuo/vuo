/**
 * @file
 * vuo.scene.explode node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoGradientNoiseCommon.h"
#include "VuoSceneObjectRenderer.h"

#include "VuoGlPool.h"
#include <Block.h>
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Explode 3D Object",
					 "keywords" : [
						 "break", "separate", "split", "slice", "divide",
						 "shatter", "fireworks", "explosion", "blow up", "burst", "implode",
						 "face", "edge", "side", "gravity", "filter" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGradientNoiseCommon",
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "ExplodeSphere.vuo" ]
					 }
				 });

static const char *pointLineVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "deform.glsl"
	\n#include "noise3D.glsl"

	// Inputs
	uniform float time;
	uniform float translationAmount;
	uniform float rotationAmount;
	uniform float chaos;
	uniform vec3 center;
	uniform float range;
	uniform vec3 gravity;

	vec3 deform(vec3 position, vec3 normal, vec2 textureCoordinate)
	{
		rotationAmount;

		vec3 pointNoise = snoise3D3D(position*1000);
		vec3 pointChaos = pointNoise * chaos;
		float distanceFromEpicenter = distance(position + pointChaos/10, center);

		vec3 newPosition = position;

		if (distanceFromEpicenter < range)
		{
			float distanceFactor = 1/(distanceFromEpicenter*distanceFromEpicenter);

			// Apply translation.
			vec3 pointVelocity = (center - position + pointChaos/10) * translationAmount * distanceFactor;
			vec3 pointOffset = pointVelocity * time;
			newPosition += pointOffset;

			// Apply gravity.
			newPosition += gravity * time * time;
		}

		return newPosition;
	}
);

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	attribute vec3 position;
	attribute vec3 normal;
	attribute vec2 textureCoordinate;
	attribute vec4 vertexColor;

	// Outputs
	varying vec3 geometryPosition;
	varying vec3 geometryNormal;
	varying vec2 geometryTextureCoordinate;
	varying vec4 geometryVertexColor;

	void main()
	{
		geometryPosition = position;
		geometryNormal = normal;
		geometryTextureCoordinate = textureCoordinate;
		geometryVertexColor = vertexColor;

		// Older systems (e.g., NVIDIA GeForce 9400M on Mac OS 10.6)
		// fall back to the software renderer if gl_Position is not initialized.
		gl_Position = vec4(0);
	}
);

static const char *geometryShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslRandom.glsl"

	// Inputs
	uniform mat4 modelviewMatrix;
	uniform mat4 modelviewMatrixInverse;
	varying in vec3 geometryPosition[3];
	varying in vec3 geometryNormal[3];
	varying in vec2 geometryTextureCoordinate[3];
	varying in vec4 geometryVertexColor[3];
	uniform float time;
	uniform float translationAmount;
	uniform float rotationAmount;
	uniform float chaos;
	uniform vec3 center;
	uniform float range;
	uniform vec3 gravity;

	// Outputs
	varying out vec3 outPosition;
	varying out vec3 outNormal;
	varying out vec2 outTextureCoordinate;
	varying out vec4 outVertexColor;

	mat4 rotationMatrix(float angle, vec3 axis)
	{
		float c = cos(angle);
		float s = sin(angle);
		float oc = 1-c;
		return mat4(oc*axis.x*axis.x + c,
					oc*axis.x*axis.y + axis.z*s,
					oc*axis.z*axis.x - axis.y*s,
					0,
					oc*axis.x*axis.y - axis.z*s,
					oc*axis.y*axis.y + c,
					oc*axis.y*axis.z + axis.x*s,
					0,
					oc*axis.z*axis.x + axis.y*s,
					oc*axis.y*axis.z - axis.x*s,
					oc*axis.z*axis.z + c,
					0,
					0,
					0,
					0,
					1);
	}

	// Some GPUs (e.g., Intel HD Graphics 4000 on @jmcc's MacBook Air) sporadically return incorrect values for acos(),
	// so provide our own implementation (polynomial approximation).
	float acos2(float x)
	{
		// Handbook of Mathematical Functions
		// M. Abramowitz and I.A. Stegun, Ed.

		// Absolute error <= 6.7e-5
		float negate = float(x < 0);
		x = abs(x);
		float ret = -0.0187293;
		ret = ret * x;
		ret = ret + 0.0742610;
		ret = ret * x;
		ret = ret - 0.2121144;
		ret = ret * x;
		ret = ret + 1.5707288;
		ret = ret * sqrt(1.0-x);
		ret = ret - 2 * negate * ret;
		return negate * 3.14159265358979 + ret;
	}

	void main()
	{
		// Transform into worldspace.
		vec3 positionInScene[3];
		for (int i = 0; i < 3; ++i)
			positionInScene[i] = (modelviewMatrix * vec4(geometryPosition[i], 1.)).xyz;


		vec3 triangleCenter = (positionInScene[0]+positionInScene[1]+positionInScene[2])/3;
		vec3 triangleNoise = VuoGlsl_random3D3D(triangleCenter) * 2. - 1.;
		vec3 triangleChaos = triangleNoise * chaos;
		float distanceFromEpicenter = distance(triangleCenter + triangleChaos/10, center);

		vec3 newPosition[3];
		for (int i = 0; i < 3; ++i)
			newPosition[i] = positionInScene[i];

		if (distanceFromEpicenter < range)
		{
			float distanceFactor = 1/(distanceFromEpicenter*distanceFromEpicenter);

			// Apply rotation.
			for (int i = 0; i < 3; ++i)
				newPosition[i] = triangleCenter + (vec4(positionInScene[i] - triangleCenter, 1) * rotationMatrix(time * (1+triangleChaos.x) * rotationAmount, normalize(mix(triangleCenter,triangleNoise,chaos)))).xyz;

			// Apply translation.
			vec3 triangleVelocity = (center - triangleCenter + triangleChaos/10) * translationAmount * distanceFactor;
			vec3 triangleOffset = triangleVelocity * time;
			for (int i = 0; i < 3; ++i)
				newPosition[i] += triangleOffset;

			// Apply gravity.
			for (int i = 0; i < 3; ++i)
				newPosition[i] += gravity * time * time;
		}

		// Transform back into modelspace.
		for (int i = 0; i < 3; ++i)
			newPosition[i] = (modelviewMatrixInverse * vec4(newPosition[i],1)).xyz;

		// Calculate two vectors in the plane of the new triangle.
		vec3 ab = newPosition[1] - newPosition[0];
		vec3 ac = newPosition[2] - newPosition[0];
		vec3 normal    = normalize(cross(ab, ac));
		vec3 tangent   = normalize(ab);
		vec3 bitangent = normalize(ac);

		// Build a rotation matrix that transforms the initial face normal into the deformed normal.
		mat3 initialToDeformedNormal = mat3(1);
		{
			// When calculating the initial face normal, omit vertex 1
			// since on curved triangle-pair surfaces (e.g., a latlon sphere) vertex 1 of each triangle pair may not be coplanar,
			// resulting in slightly-differing initialFaceNormals in situations where they look like they should be identical.
			vec3 initialFaceNormal = normalize(geometryNormal[0] + /*geometryNormal[1] +*/ geometryNormal[2]);

			vec3 axis = normalize(cross(initialFaceNormal, normal));
			float angle = acos2(dot(initialFaceNormal, normal));

			// NVIDIA 9400M apparently NANs on angles closer to zero.
			// https://b33p.net/kosada/node/11850
			if (angle > /*0.0001*/ 0.001)
			{
				// Turn the axis/angle into a quaternion.
				vec4 q;
				q.x = axis.x * sin(angle/2.f);
				q.y = axis.y * sin(angle/2.f);
				q.z = axis.z * sin(angle/2.f);
				q.w = cos(angle/2.f);

				// Turn the quaternion into a matrix.
				initialToDeformedNormal[0][0] = 1. - 2.*(q.y*q.y + q.z*q.z);
				initialToDeformedNormal[0][1] =      2.*(q.x*q.y + q.w*q.z);
				initialToDeformedNormal[0][2] =      2.*(q.x*q.z - q.w*q.y);
				initialToDeformedNormal[1][0] =      2.*(q.x*q.y - q.w*q.z);
				initialToDeformedNormal[1][1] = 1. - 2.*(q.x*q.x + q.z*q.z);
				initialToDeformedNormal[1][2] =      2.*(q.y*q.z + q.w*q.x);
				initialToDeformedNormal[2][0] =      2.*(q.x*q.z + q.w*q.y);
				initialToDeformedNormal[2][1] =      2.*(q.y*q.z - q.w*q.x);
				initialToDeformedNormal[2][2] = 1. - 2.*(q.x*q.x + q.y*q.y);
			}
		}

		// Emit the vertices with the calculated normal.
		for (int i = 0; i < 3; ++i)
		{
			outPosition = newPosition[i];

			// Rotate the original normal toward the deformed normal (to preserve smoothing).
			outNormal = normalize(geometryNormal[i].xyz * initialToDeformedNormal);

			outTextureCoordinate = geometryTextureCoordinate[i];
			outVertexColor = geometryVertexColor[i];

			EmitVertex();
		}
		EndPrimitive();
	}
);

struct nodeInstanceData
{
	VuoShader shader;
	VuoSceneObjectRenderer sceneObjectRenderer;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_make("Explode Object");
	VuoShader_addSource(instance->shader, VuoMesh_Points,              pointLineVertexShaderSource, NULL,                 NULL);
	VuoShader_addSource(instance->shader, VuoMesh_IndividualLines,     pointLineVertexShaderSource, NULL,                 NULL);
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, vertexShaderSource,          geometryShaderSource, NULL);
	VuoRetain(instance->shader);

	instance->sceneObjectRenderer = VuoSceneObjectRenderer_make(instance->shader);
	VuoRetain(instance->sceneObjectRenderer);

	return instance;
}

/**
 * Improved canonical pseudorandomness (minus the `highp` improvement which isn't supported on GLSL 1.20).
 *
 * Each function returns values between 0 and 1.
 *
 * https://web.archive.org/web/20141130173831/http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0
 */
static float vuo_scene_explode_random3D1D(VuoPoint3d co)
{
	float c  = 43758.5453;
	float dt = co.x * 12.9898 + co.y * 78.233 + co.z * 469.398;
	float sn = dt - 3.14 * floorf(dt / 3.14);
	float f  = sinf(sn) * c;
	return f - floorf(f);
}

static VuoPoint3d vuo_scene_explode_random3D3D(VuoPoint3d v)
{
	return (VuoPoint3d){vuo_scene_explode_random3D1D((VuoPoint3d){v.x-1, v.y-1, v.z-1}),
						vuo_scene_explode_random3D1D((VuoPoint3d){v.x,   v.y,   v.z  }),
						vuo_scene_explode_random3D1D((VuoPoint3d){v.x+1, v.y+1, v.z+1})};
}


void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoSceneObject) object,
		VuoInputData(VuoReal) time,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedMax":2.0}) translationAmount,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedMax":2.0}) rotationAmount,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedMax":2.0}) chaos,
		VuoInputData(VuoPoint3d, {"default":{"x":0.0,"y":0.0,"z":0.0}, "suggestedMin":{"x":-1.0,"y":-1.0,"z":-1.0}, "suggestedMax":{"x":1.0,"y":1.0,"z":1.0}, "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) center,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedMax":2.0}) range,
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":-1,"z":0}}) gravity,
		VuoOutputData(VuoSceneObject) explodedObject
)
{
	float translationAmountScaled = translationAmount / -20.;
	float rotationAmountScaled = rotationAmount * 2;
	float chaos2 = chaos * 2;
	VuoPoint3d gravityScaled = gravity / (VuoPoint3d)(10);

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "time",              time);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "translationAmount", translationAmountScaled);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "rotationAmount",    rotationAmountScaled);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "chaos",             chaos2);
	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "center",            center);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "range",             range);
	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "gravity",           gravityScaled);

	// Render.
	*explodedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object,
		^(float *modelMatrix, float *modelMatrixInverse, int *vertexCount, float *positions, float *normals, float *textureCoordinates, float *colors) {
			if (*vertexCount < 3)
			{
				for (int i = 0; i < *vertexCount; ++i)
				{
					VuoPoint3d position         = VuoPoint3d_makeFromArray(&positions[i * 3]);
					VuoPoint3d pointNoise       = VuoGradientNoise_simplex_VuoPoint3d_VuoPoint3d(position * (VuoPoint3d)(1000));
					VuoPoint3d pointChaos       = pointNoise * chaos2;
					float distanceFromEpicenter = VuoPoint3d_distance(position + pointChaos / (VuoPoint3d)(10.), center);

					if (distanceFromEpicenter < range)
					{
						float distanceFactor = 1 / (distanceFromEpicenter * distanceFromEpicenter);

						// Apply translation.
						VuoPoint3d pointVelocity = (center - position + pointChaos / (VuoPoint3d)(10.)) * translationAmountScaled * distanceFactor;
						VuoPoint3d pointOffset   = pointVelocity * (VuoPoint3d)(time);
						VuoPoint3d newPosition = position + pointOffset;

						// Apply gravity.
						newPosition += gravityScaled * (VuoPoint3d)(time * time);

						VuoPoint3d_setArray(&positions[i * 3], newPosition);
					}
				}
			}
			else
			{
				// Transform into worldspace.
				VuoPoint3d positionInScene[3];
				for (int i = 0; i < 3; ++i)
				{
					VuoPoint3d position = VuoPoint3d_makeFromArray(&positions[i * 3]);
					positionInScene[i] = VuoTransform_transformPoint(modelMatrix, position);
				}

				VuoPoint3d triangleCenter   = (positionInScene[0] + positionInScene[1] + positionInScene[2]) / (VuoPoint3d)(3);
				VuoPoint3d triangleNoise    = vuo_scene_explode_random3D3D(triangleCenter) * (VuoPoint3d)(2.) - (VuoPoint3d)(1);
				VuoPoint3d triangleChaos    = triangleNoise * chaos2;
				float distanceFromEpicenter = VuoPoint3d_distance(triangleCenter + triangleChaos / (VuoPoint3d)(10), center);

				VuoPoint3d newPosition[3];
				for (int i = 0; i < 3; ++i)
					newPosition[i] = positionInScene[i];

				if (distanceFromEpicenter < range)
				{
					float distanceFactor = 1 / (distanceFromEpicenter * distanceFromEpicenter);

					// Apply rotation.
					VuoPoint4d q = VuoTransform_quaternionFromAxisAngle(VuoPoint3d_normalize(VuoPoint3d_lerp(triangleCenter, triangleNoise, chaos2)),
															  time * (1 + triangleChaos.x) * rotationAmountScaled);
					VuoTransform t = VuoTransform_makeQuaternion((VuoPoint3d){0,0,0}, q, (VuoPoint3d){1,1,1});
					float matrix[16];
					VuoTransform_getMatrix(t, matrix);
					for (int i = 0; i < 3; ++i)
						newPosition[i] = triangleCenter + VuoTransform_transformPoint(matrix, positionInScene[i] - triangleCenter);

					// Apply translation.
					VuoPoint3d triangleVelocity = (center - triangleCenter + triangleChaos / (VuoPoint3d)(10)) * (VuoPoint3d)(translationAmountScaled * distanceFactor);
					VuoPoint3d triangleOffset   = triangleVelocity * (VuoPoint3d)(time);
					for (int i = 0; i < 3; ++i)
						newPosition[i] += triangleOffset;

					// Apply gravity.
					for (int i = 0; i < 3; ++i)
						newPosition[i] += gravityScaled * (VuoPoint3d)(time * time);
				}

				// Transform back into modelspace.
				for (int i = 0; i < 3; ++i)
					newPosition[i] = VuoTransform_transformPoint(modelMatrixInverse, newPosition[i]);

				// Calculate two vectors in the plane of the new triangle.
				VuoPoint3d ab        = newPosition[1] - newPosition[0];
				VuoPoint3d ac        = newPosition[2] - newPosition[0];
				VuoPoint3d normal    = VuoPoint3d_normalize(VuoPoint3d_crossProduct(ab, ac));

				// Build a rotation matrix that transforms the initial face normal into the deformed normal.
				float initialToDeformedNormal[16];
				{
					// When calculating the initial face normal, omit vertex 1
					// since on curved triangle-pair surfaces (e.g., a latlon sphere) vertex 1 of each triangle pair may not be coplanar,
					// resulting in slightly-differing initialFaceNormals in situations where they look like they should be identical.
					VuoPoint3d normal0 = VuoPoint3d_makeFromArray(&normals[0 * 3]);
					VuoPoint3d normal2 = VuoPoint3d_makeFromArray(&normals[2 * 3]);
					VuoPoint3d initialFaceNormal = VuoPoint3d_normalize(normal0 + normal2);

					VuoPoint4d q = VuoTransform_quaternionFromVectors(initialFaceNormal, normal);
					VuoTransform t = VuoTransform_makeQuaternion((VuoPoint3d){0,0,0}, q, (VuoPoint3d){1,1,1});
					VuoTransform_getMatrix(t, initialToDeformedNormal);
				}

				for (int i = 0; i < 3; ++i)
				{
					VuoPoint3d_setArray(&positions[i * 3], newPosition[i]);

					// Rotate the original normal toward the deformed normal (to preserve smoothing).
					VuoPoint3d normal0 = VuoPoint3d_makeFromArray(&normals[i * 3]);
					VuoPoint3d rotatedNormal = VuoTransform_transformPoint(initialToDeformedNormal, normal0);
					VuoPoint3d_setArray(&normals[i * 3], rotatedNormal);
				}
			}
		});
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
}
