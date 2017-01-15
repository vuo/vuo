/**
 * @file
 * example.imageFilter.brightness node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->glContext = VuoGlContext_use();

	instance->shader = VuoShader_make("Adjust Color Brightness Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoRetain(instance->shader);

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
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

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "red",     red);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "green",   green);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "blue",    blue);

	// Render.
	*adjustedImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
