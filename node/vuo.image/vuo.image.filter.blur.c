/**
 * @file
 * vuo.image.filter.blur node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Blur Image",
					 "keywords" : [ "gaussian", "distort", "obscure", "smudge" ],
					 "version" : "1.0.0",
				 });

// Multiple pass Gaussian blur implementation adapted from:
// http://www.geeks3d.com/20100909/shader-library-gaussian-blur-post-processing-filter-in-glsl/
static const char * verticalPassFragmentShader = VUOSHADER_GLSL_SOURCE(120,

	// uniform float width;
	varying vec4 fragmentTextureCoordinate;

	uniform sampler2D texture;

	uniform float height; 	// render target height

	float offset[3] = float[]( 0.0, 1.3846153846, 3.2307692308 );
	float weight[3] = float[]( 0.2270270270, 0.3162162162, 0.0702702703 );

	void main(void)
	{
		vec2 uv = fragmentTextureCoordinate.xy;
		vec4 tc = texture2D(texture, uv);
		tc.rgb *= weight[0];
		
		for (int i=1; i<3; i++) 
		{
			tc.rgb += texture2D(texture, uv + vec2(0.0, offset[i])/height).rgb * weight[i];
			tc.rgb += texture2D(texture, uv - vec2(0.0, offset[i])/height).rgb * weight[i];
		}

		gl_FragColor = tc;
	}
);

static const char * horizontalPassFragmentShader = VUOSHADER_GLSL_SOURCE(120,

	varying vec4 fragmentTextureCoordinate;

	uniform sampler2D texture;

	uniform float width; 	// render target width

	float offset[3] = float[]( 0.0, 1.3846153846, 3.2307692308 );
	float weight[3] = float[]( 0.2270270270, 0.3162162162, 0.0702702703 );

	void main(void)
	{
		vec2 uv = fragmentTextureCoordinate.xy;
		vec4 tc = texture2D(texture, uv);
		tc.rgb *= weight[0];
		
		for (int i=1; i<3; i++) 
		{
			tc.rgb += texture2D(texture, uv + vec2(offset[i])/width, 0.0).rgb * weight[i];
			tc.rgb += texture2D(texture, uv - vec2(offset[i])/width, 0.0).rgb * weight[i];
		}

		gl_FragColor = tc;
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
		VuoInputData(VuoInteger, {"default":4, "suggestedMin":0, "suggestedMax":20}) radius,
		VuoOutputData(VuoImage) blurredImage
)
{
	if (!image)
		return;

	int w = image->pixelsWide;
	int h = image->pixelsHigh;

	// set img to unmodified input image (a radius of '0' should output an unmodified texture)
	VuoImage img = image;

	VuoShader verticalPassShader = VuoShader_make("Gaussian Vertical Pass Shader", VuoShader_getDefaultVertexShader(), verticalPassFragmentShader);
	VuoShader horizontalPassShader = VuoShader_make("Gaussian Horizontal Pass Shader", VuoShader_getDefaultVertexShader(), horizontalPassFragmentShader);
	VuoRetain(verticalPassShader);
	VuoRetain(horizontalPassShader);
	VuoShader_setUniformFloat(verticalPassShader, (*instance)->glContext, "height", h);
	VuoShader_setUniformFloat(horizontalPassShader, (*instance)->glContext, "width", w);

	for(int i = 0; i < radius; i++)
	{
		// apply vertical pass
		VuoShader_resetTextures(verticalPassShader);
		VuoShader_addTexture(verticalPassShader, (*instance)->glContext, "texture", img);

		VuoImage verticalPassImage = VuoImageRenderer_draw((*instance)->imageRenderer, verticalPassShader, w, h);

		// apply horizontal pass
		VuoShader_resetTextures(horizontalPassShader);
		VuoShader_addTexture(horizontalPassShader, (*instance)->glContext, "texture", verticalPassImage);
		
		// one pass complete, ready for another (or not)
		img = VuoImageRenderer_draw((*instance)->imageRenderer, verticalPassShader, w, h);
	}

	// output!
	*blurredImage = img;

	VuoRelease(verticalPassShader);
	VuoRelease(horizontalPassShader);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
