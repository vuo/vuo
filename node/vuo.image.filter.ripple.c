/**
 * @file
 * vuo.image.filter.ripple node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Ripple Image",
					 "description" :
						"<p>Distorts the image with a wave or ripple effect.</p> \
						<p>This node applies sinusoidal ripples to the image. The ripples can be animated to move across the image \
						by sending gradually increasing values to the `phase` port.</p> \
						<p><ul> \
						<li>`angle` — The angle or direction of the ripples, in degrees. \
						At 0, the ripples move side to side as `phase` increases. At 90, the ripples move up and down.</li> \
						<li>`amplitude` — The amount that each ripple distorts the image. At 0, there is no distortion.</li> \
						<li>`wavelength` — The size of each ripple, in Vuo coordinates. At 0, the output image is black.</li> \
						<li>`phase` — The current time in the wave cycle. At 2π (about 6.28), the phase is back to the beginning of the cycle.</li> \
						</ul></p> \
						<p>In Vuo coordinates, (0,0) is the center of the image. \
						The image has a width of 2, with x-coordinate -1 on the left edge and 1 on the right edge. \
						The image's height is determined by its aspect ratio, with the y-coordinate increasing from bottom to top.</p>",
					 "keywords" : [ "wave", "sinusoidal", "sine", "cosine", "undulate", "ruffle", "swish", "swing", "wag", "flap", "sway", "billow", "water" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoImageRenderer"
					 ],
					 "node": {
						 "isInterface" : false
					 }
				 });


static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	uniform sampler2D texture;
	uniform float angle;
	uniform float amplitude;
	uniform float wavelength;
	uniform float phase;
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
		float samplerPhase = cos(angle)*fragmentTextureCoordinate.x + sin(angle)*fragmentTextureCoordinate.y;
		float offset = sin(samplerPhase/wavelength + phase) * amplitude;
		gl_FragColor = texture2D(texture, fragmentTextureCoordinate.xy + vec2(cos(angle)*offset,sin(angle)*offset));
	}
);


struct nodeInstanceData
{
	VuoShader shader;
	VuoImageRenderer imageRenderer;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_make("Ripple Image", VuoShader_getDefaultVertexShader(), fragmentShaderSource);
	VuoRetain(instance->shader);
	instance->imageRenderer = VuoImageRenderer_make();
	VuoRetain(instance->imageRenderer);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":135,"suggestedMin":0,"suggestedMax":360,"suggestedStep":1}) angle,
		VuoInputData(VuoReal, {"default":0.1,"suggestedMin":0,"suggestedMax":1}) amplitude,
		VuoInputData(VuoReal, {"default":0.05,"suggestedMin":0.00001,"suggestedMax":0.05}) wavelength,
		VuoInputData(VuoReal, {"default":0,"suggestedMin":0,"suggestedMax":1}) phase,
		VuoOutputData(VuoImage) rippledImage
)
{
	if (! image)
		return;

	// Associate the input image with the shader.
	VuoShader_resetTextures((*instance)->shader);
	VuoShader_addTexture((*instance)->shader, image, "texture");

	// Feed parameters to the shader.
	VuoShader_setUniformFloat((*instance)->shader, "angle", angle*M_PI/180.);
	VuoShader_setUniformFloat((*instance)->shader, "amplitude", amplitude);
	VuoShader_setUniformFloat((*instance)->shader, "wavelength", wavelength*M_PI*2.);
	VuoShader_setUniformFloat((*instance)->shader, "phase", phase*M_PI*2.);

	// Render.
	*rippledImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, image->pixelsWide, image->pixelsHigh);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
}
