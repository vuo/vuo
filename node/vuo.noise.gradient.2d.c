/**
 * @file
 * vuo.noise.gradient.2d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGradientNoiseCommon.h"

VuoModuleMetadata({
					 "title" : "Make Gradient Noise 2D",
					 "description" :
						"<p>Generates a pseudorandom number using Perlin noise or simplex noise.</p> \
						<p>Gradient noise is useful for creating natural-looking movement and textures. \
						This node can be used to animate an object in a natural-looking motion \
						by sending gradually changing values to the `position` port.</p> \
						<p><ul> \
						<li>`position` – For a given position, the gradient noise value is always the same. \
						Typically you would want to send into this input port a series of points, with each \
						point a small distance (less than 1) from the previous one. When the point's coordinates \
						are integers, the gradient noise value is 0.</li> \
						<li>`gradientNoise` — The way to generate the gradient noise. <ul> \
						<li>\"Perlin\" — improved Perlin noise</li> \
						<li>\"Simplex\" — simplex noise</li> \
						</ul></li> \
						<li>`value` — The gradient noise value, usually ranging from -1 to 1.</li> \
						</ul></p>",
					 "keywords" : [ "perlin", "simplex", "random", "pseudo", "natural" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGradientNoiseCommon"
					 ],
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}}) position,
	VuoInputData(VuoGradientNoise, {"default":"perlin"}) gradientNoise,
	VuoOutputData(VuoReal) value
)
{
	if (gradientNoise == VuoGradientNoise_Perlin)
		*value = VuoGradientNoise_perlin2D(position.x, position.y);
	else if (gradientNoise == VuoGradientNoise_Simplex)
		*value = VuoGradientNoise_simplex2D(position.x, position.y);
}
