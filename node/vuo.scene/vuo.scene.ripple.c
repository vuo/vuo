/**
 * @file
 * vuo.scene.ripple node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoSceneObjectRenderer.h"

#include "VuoGlPool.h"
#include <OpenGL/CGLMacro.h>

#include "VuoDispersion.h"
#include "VuoDisplacement.h"

VuoModuleMetadata({
					 "title" : "Ripple 3D Object",
					 "keywords" : [
						 "wave",
						 "transverse", "t-wave", "shear", "ruffle", "swish", "swing", "flap", "sway", "water",
						 "longitudinal", "l-wave", "compressional", "pressure", "rarefaction", "slinky",
						 "sinusoidal", "sine", "cosine", "undulate", "displace", "filter",
						 "linear", "radial", "circular", "spherical"
					 ],
					 "version" : "1.0.1",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "MoveBeadsOnString.vuo", "RippleGrid.vuo" ]
					 }
				 });

static const char *linearShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(deform)

	// Inputs
	uniform float angle;
	uniform float angleDelta;
	uniform float amplitude;
	uniform float wavelength;
	uniform float phase;

	vec3 deform(vec3 position)
	{
		float coordinatePhase = position.x*cos(angle) + position.y*sin(angle);
		float offset = sin(coordinatePhase/wavelength + phase) * amplitude;
		return position + offset * vec3(cos(angle+angleDelta), sin(angle+angleDelta), 0);
	}
);

static const char *radialLongitudinalShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(deform)

	// Inputs
	uniform float angle;
	uniform float amplitude;
	uniform float wavelength;
	uniform float phase;

	vec3 deform(vec3 position)
	{
		float coordinatePhase = sqrt(position.x*position.x + position.y*position.y + position.z*position.z);
		float offset = sin(coordinatePhase/wavelength - phase) * amplitude;
		float theta = atan(position.y, position.x);
		return position + offset * vec3(cos(theta), sin(theta)*cos(angle), sin(angle));
	}
);

static const char *radialTransverseShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(deform)

	// Inputs
	uniform float angle;
	uniform float amplitude;
	uniform float wavelength;
	uniform float phase;

	vec3 deform(vec3 position)
	{
		float coordinatePhase = sqrt(position.x*position.x + position.y*position.y + position.z*position.z);
		float offset = sin(coordinatePhase/wavelength - phase) * amplitude;
		return position + offset * vec3(0, cos(angle+3.14159/2), sin(angle+3.14159/2));
	}
);

struct nodeInstanceData
{
	VuoDispersion dispersion;
	VuoDisplacement displacement;
	VuoShader shader;

	VuoGlContext glContext;
	VuoSceneObjectRenderer sceneObjectRenderer;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->dispersion = VuoDispersion_Linear;
	instance->displacement = VuoDisplacement_Transverse;
	instance->shader = VuoShader_make("Ripple Object (Linear)");
	VuoShader_addSource(instance->shader, VuoMesh_Points,              linearShaderSource, NULL, NULL);
	VuoShader_addSource(instance->shader, VuoMesh_IndividualLines,     linearShaderSource, NULL, NULL);
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, linearShaderSource, NULL, NULL);
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
		VuoInputData(VuoReal, {"default":135.0,"suggestedMin":0,"suggestedMax":360,"suggestedStep":15}) angle,
		VuoInputData(VuoReal, {"default":0.05,"suggestedMin":0,"suggestedMax":0.2}) amplitude,
		VuoInputData(VuoReal, {"default":0.02,"suggestedMin":0.000001,"suggestedMax":0.04}) wavelength,
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1}) phase,
		VuoInputData(VuoDispersion, {"default":"linear"}) dispersion,
		VuoInputData(VuoDisplacement, {"default":"transverse"}) displacement,
		VuoOutputData(VuoSceneObject) rippledObject
)
{
	if ((*instance)->dispersion != dispersion
			|| (dispersion == VuoDispersion_Radial && (*instance)->displacement != displacement))
	{
		// Switch shaders.
		VuoRelease((*instance)->shader);

		if (dispersion == VuoDispersion_Linear)
		{
			(*instance)->shader = VuoShader_make("Ripple Object (Linear)");
			VuoShader_addSource((*instance)->shader, VuoMesh_Points,              linearShaderSource, NULL, NULL);
			VuoShader_addSource((*instance)->shader, VuoMesh_IndividualLines,     linearShaderSource, NULL, NULL);
			VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, linearShaderSource, NULL, NULL);
		}
		else if (dispersion == VuoDispersion_Radial)
		{
			if (displacement == VuoDisplacement_Longitudinal)
			{
				(*instance)->shader = VuoShader_make("Ripple Object (Radial Longitudinal)");
				VuoShader_addSource((*instance)->shader, VuoMesh_Points,              radialLongitudinalShaderSource, NULL, NULL);
				VuoShader_addSource((*instance)->shader, VuoMesh_IndividualLines,     radialLongitudinalShaderSource, NULL, NULL);
				VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, radialLongitudinalShaderSource, NULL, NULL);
			}
			else
			{
				(*instance)->shader = VuoShader_make("Ripple Object (Radial Transverse)");
				VuoShader_addSource((*instance)->shader, VuoMesh_Points,              radialTransverseShaderSource, NULL, NULL);
				VuoShader_addSource((*instance)->shader, VuoMesh_IndividualLines,     radialTransverseShaderSource, NULL, NULL);
				VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, radialTransverseShaderSource, NULL, NULL);
			}
		}

		(*instance)->dispersion = dispersion;
		(*instance)->displacement = displacement;
		VuoRetain((*instance)->shader);

		VuoRelease((*instance)->sceneObjectRenderer);
		(*instance)->sceneObjectRenderer = VuoSceneObjectRenderer_make((*instance)->glContext, (*instance)->shader);
		VuoRetain((*instance)->sceneObjectRenderer);
	}

	double nonzeroWavelength = VuoReal_makeNonzero(wavelength);

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal    ((*instance)->shader, "angle",      angle*M_PI/180.);
	if (dispersion == VuoDispersion_Linear)
		VuoShader_setUniform_VuoReal((*instance)->shader, "angleDelta", displacement == VuoDisplacement_Transverse ? M_PI/2. : 0);
	VuoShader_setUniform_VuoReal    ((*instance)->shader, "amplitude",  amplitude);
	VuoShader_setUniform_VuoReal    ((*instance)->shader, "wavelength", nonzeroWavelength*M_PI*2.);
	VuoShader_setUniform_VuoReal    ((*instance)->shader, "phase",      phase*M_PI*2.);

	// Render.
	*rippledObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
