/**
 * @file
 * vuo.scene.explode node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoSceneObjectRenderer.h"

#include "VuoGlPool.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Explode 3D Object",
					 "keywords" : [ "break", "separate", "shatter", "fireworks", "explosion", "blow up", "burst",
						 "face", "edge", "side", "gravity", "filter" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "ExplodeSphere.vuo" ]
					 }
				 });

static const char *pointLineVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(deform)
	include(noise3D)

	// Inputs
	uniform float time;
	uniform float translationAmount;
	uniform float rotationAmount;
	uniform float chaos;
	uniform vec3 center;
	uniform float range;
	uniform vec3 gravity;

	vec3 deform(vec3 position)
	{
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
	attribute vec4 position;
	attribute vec4 normal;
	attribute vec4 textureCoordinate;

	// Outputs
	varying vec4 geometryPosition;
	varying vec4 geometryNormal;
	varying vec4 geometryTextureCoordinate;

	void main()
	{
		geometryPosition = position;
		geometryNormal = normal;
		geometryTextureCoordinate = textureCoordinate;

		// Older systems (e.g., NVIDIA GeForce 9400M on Mac OS 10.6)
		// fall back to the software renderer if gl_Position is not initialized.
		gl_Position = vec4(0);
	}
);

static const char *geometryShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslRandom)

	// Inputs
	uniform mat4 modelviewMatrix;
	uniform mat4 modelviewMatrixInverse;
	varying in vec4 geometryPosition[3];
	varying in vec4 geometryNormal[3];
	varying in vec4 geometryTextureCoordinate[3];
	uniform float time;
	uniform float translationAmount;
	uniform float rotationAmount;
	uniform float chaos;
	uniform vec3 center;
	uniform float range;
	uniform vec3 gravity;

	// Outputs
	varying out vec4 outPosition;
	varying out vec4 outNormal;
	varying out vec4 outTangent;
	varying out vec4 outBitangent;
	varying out vec4 outTextureCoordinate;

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

	// Some GPUs (e.g., Intel HD Graphics 4000 on @jmcc's MacBook Air) sporadically return crazy values for acos(),
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
			positionInScene[i] = (modelviewMatrix * geometryPosition[i]).xyz;


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
		vec4 normal    = vec4(normalize(cross(ab, ac)),1);
		vec4 tangent   = vec4(normalize(ab),1);
		vec4 bitangent = vec4(normalize(ac),1);

		// Build a rotation matrix that transforms the initial face normal into the deformed normal.
		mat3 initialToDeformedNormal = mat3(1);
		{
			// When calculating the initial face normal, omit vertex 1
			// since on curved triangle-pair surfaces (e.g., a latlon sphere) vertex 1 of each triangle pair may not be coplanar,
			// resulting in slightly-differing initialFaceNormals in situations where they look like they should be identical.
			vec3 initialFaceNormal = normalize(geometryNormal[0].xyz + /*geometryNormal[1].xyz +*/ geometryNormal[2].xyz);

			vec3 axis = normalize(cross(initialFaceNormal, normal.xyz));
			float angle = acos2(dot(initialFaceNormal, normal.xyz));
			if (angle > 0.0001)
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
			outPosition = vec4(newPosition[i],1);

			// Rotate the original normal toward the deformed normal (to preserve smoothing).
			outNormal = vec4(normalize(geometryNormal[i].xyz * initialToDeformedNormal), 1);

			outTangent = tangent;
			outBitangent = bitangent;
			outTextureCoordinate = geometryTextureCoordinate[i];
			EmitVertex();
		}
		EndPrimitive();
	}
);

struct nodeInstanceData
{
	VuoShader shader;
	VuoGlContext glContext;
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

	instance->glContext = VuoGlContext_use();

	instance->sceneObjectRenderer = VuoSceneObjectRenderer_make(instance->glContext, instance->shader);
	VuoRetain(instance->sceneObjectRenderer);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoSceneObject) object,
		VuoInputData(VuoReal) time,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedMax":2.0}) translationAmount,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedMax":2.0}) rotationAmount,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedMax":2.0}) chaos,
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0},"suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) center,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedMax":2.0}) range,
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":-1,"z":0}}) gravity,
		VuoOutputData(VuoSceneObject) explodedObject
)
{
	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "time",              time);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "translationAmount", translationAmount/-20.);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "rotationAmount",    rotationAmount*2);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "chaos",             chaos*2);
	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "center",            center);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "range",             range);
	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "gravity",           VuoPoint3d_multiply(gravity, 1./10.));

	// Render.
	*explodedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
