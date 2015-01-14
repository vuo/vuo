/**
 * @file
 * test.adjustColorBrightness node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Adjust Color Brightness",
					 "description" : "Adjusts the brightness of each color channel individually.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoImageRenderer"
					 ],
					 "node": {
						 "isInterface" : false
					 }
				 });

#include "node.h"

static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	uniform sampler2D texture;
	uniform float red;
	uniform float green;
	uniform float blue;
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
		vec4 frag = texture2D(texture, fragmentTextureCoordinate.xy);
		gl_FragColor = vec4(frag.r*red, frag.g*green, frag.b*blue, frag.a);
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

	instance->shader = VuoShader_make("Swap Color Channels", VuoShader_getDefaultVertexShader(), fragmentShaderSource);
	VuoRetain(instance->shader);
	instance->imageRenderer = VuoImageRenderer_make();
	VuoRetain(instance->imageRenderer);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":1,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) red,
		VuoInputData(VuoReal, {"default":1,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) green,
		VuoInputData(VuoReal, {"default":1,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) blue,
		VuoOutputData(VuoImage) adjustedImage
)
{
	if (! image)
		return;

	{
		VuoGlContext glContext = VuoGlContext_use();

		// Associate the input image with the shader.
		VuoShader_resetTextures((*instance)->shader);
		VuoShader_addTexture((*instance)->shader, glContext, "texture", image);

		// Feed parameters to the shader.
		VuoShader_setUniformFloat((*instance)->shader, glContext, "red", red);
		VuoShader_setUniformFloat((*instance)->shader, glContext, "green", green);
		VuoShader_setUniformFloat((*instance)->shader, glContext, "blue", blue);

		// Render.
		*adjustedImage = VuoImageRenderer_draw((*instance)->imageRenderer, glContext, (*instance)->shader, image->pixelsWide, image->pixelsHigh);

		VuoGlContext_disuse(glContext);
	}
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
}
