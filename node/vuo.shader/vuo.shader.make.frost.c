/**
 * @file
 * vuo.shader.make.frost node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make Frosted Glass Shader",
					  "keywords" : [
						  "texture",
						  "paint", "draw", "opengl", "glsl", "scenegraph", "graphics",
						  "perlin", "simplex", "gradient",
						  "blur", "bend", "tint", "refraction", "diffraction",
					  ],
					  "version" : "1.2.1",
					  "node" : {
						  "exampleCompositions" : [ "ShowSphereThroughWarpedPlastic.vuo" ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoColor, {"default":{"r":0.88,"g":0.93,"b":1.,"a":1.}}) color,
		VuoInputData(VuoReal, {"default":1.5, "suggestedMin":0., "suggestedMax":2., "suggestedStep":0.1}) brightness,
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) noiseCenter,
		VuoInputData(VuoReal, {"default":0.0, "suggestedStep":0.1}) noiseTime,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0., "suggestedMax":1., "suggestedStep":0.1}) noiseAmount,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0., "suggestedMax":1., "suggestedStep":0.1}) noiseScale,
		VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0., "suggestedMax":1., "suggestedStep":0.1}) chromaticAberration,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":4}) levels,
		VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) roughness,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":1.0, "suggestedMax":5.0, "suggestedStep":0.1}) spacing,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":8}) iterations,
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_makeFrostedGlassShader();
	VuoShader_setFrostedGlassShaderValues(*shader, color, brightness, noiseCenter, noiseTime, noiseAmount, noiseScale, chromaticAberration, levels, roughness, spacing, iterations, 1);
}
