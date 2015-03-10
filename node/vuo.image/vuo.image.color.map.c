/**
 * @file
 * vuo.image.color.map node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					  "title" : "Map Image Colors",
					  "keywords" : [ "gradient", "replace", "heatmap", "tint", "tone", "chroma", "recolor", "colorize", "correction", "calibration", "grading", "balance", "filter" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "RecolorMovie.vuo" ]
					  }
				 });

static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,

	uniform float gradientCount;
	uniform sampler2D image;		// the unfiltered image
	uniform sampler2D gradientStrip;// the gradient strip to map against
	uniform float amount;			// the amount to mix gradient and image
	varying vec4 fragmentTextureCoordinate;

	// https://stackoverflow.com/questions/596216/formula-to-determine-brightness-of-rgb-color
	float brightness(vec3 col)
	{
		// Digital CCIR601 (gives more weight to the green and red components):
		return 0.299*col.r + 0.587*col.g + 0.114*col.b;
	}

	void main(void)
	{
		vec4 orig = texture2D(image, fragmentTextureCoordinate.xy);

		float lum = brightness(orig.rgb);

		float gradientWidth = (1./gradientCount)/2.;
		lum = lum * (1-gradientWidth*2) + gradientWidth;

		vec4 color = texture2D(gradientStrip, vec2(clamp(lum, gradientWidth, 1-gradientWidth), .5));

		gl_FragColor = mix(orig, color, amount);
	}
);

struct nodeInstanceData
{
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoList_VuoColor, {"default":[
			{"r":0,"g":1,"b":0,"a":1},	// green
			{"r":1,"g":0,"b":0,"a":1},	// red
			{"r":1,"g":1,"b":0,"a":1}]})// yellow
			colors,
		VuoInputEvent(VuoPortEventBlocking_None, colors) colorsEvent,
		VuoInputData(VuoReal, {"default":1,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) filterOpacity,
		VuoOutputData(VuoImage) mappedImage
)
{
	if (!image)
		return;

	if (!colors)
	{
		*mappedImage = image;
		return;
	}

	int len = VuoListGetCount_VuoColor(colors);

	unsigned char* pixels = (unsigned char*)malloc(sizeof(char)*len*4);
	int n = 0;
	for(int i = 1; i <= len; i++)
	{
		VuoColor col = VuoListGetValueAtIndex_VuoColor(colors, i);
		pixels[n++] = (unsigned int)(col.r*255);
		pixels[n++] = (unsigned int)(col.g*255);
		pixels[n++] = (unsigned int)(col.b*255);
		pixels[n++] = (unsigned int)(col.a*255);
	}

	VuoImage gradientStrip = VuoImage_makeFromBuffer(pixels, GL_RGBA, len, 1);

	VuoShader shader = VuoShader_make("Map Image Colors", VuoShader_getDefaultVertexShader(), fragmentShaderSource);
	VuoRetain(shader);

	VuoShader_resetTextures(shader);
	VuoShader_addTexture(shader, (*instance)->glContext, "gradientStrip", gradientStrip);
	VuoShader_addTexture(shader, (*instance)->glContext, "image", image);
	VuoShader_setUniformFloat(shader, (*instance)->glContext, "gradientCount", (float)len);
	VuoShader_setUniformFloat(shader, (*instance)->glContext, "amount", filterOpacity);

	// Render.
	*mappedImage = VuoImageRenderer_draw((*instance)->imageRenderer, shader, image->pixelsWide, image->pixelsHigh);

	VuoRelease(shader);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
