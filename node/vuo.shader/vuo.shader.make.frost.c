/**
 * @file
 * vuo.shader.make.frost node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Shade with Frosted Glass",
					  "keywords" : [ "texture", "paint", "draw", "opengl", "glsl", "scenegraph", "graphics",
						  "blur", "bend", "tint", "refraction", "diffraction" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "ShowSphereThroughWarpedPlastic.vuo" ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoColor, {"default":{"r":0.88,"g":0.93,"b":1.,"a":1.}}) color,
		VuoInputData(VuoReal, {"default":1.5, "suggestedMin":0., "suggestedMax":2., "suggestedStep":0.1}) brightness,
		VuoInputData(VuoReal, {"default":0.0, "suggestedStep":0.1}) noiseTime,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0., "suggestedMax":1., "suggestedStep":0.1}) noiseAmount,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0., "suggestedMax":1., "suggestedStep":0.1}) noiseScale,
		VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0., "suggestedMax":1., "suggestedStep":0.1}) chromaticAberration,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":50}) iterations,
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_makeFrostedGlassShader();
	VuoShader_setFrostedGlassShaderValues(*shader, color, brightness, noiseTime, noiseAmount, noiseScale, chromaticAberration, iterations);
}
