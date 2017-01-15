/**
 * @file
 * vuo.image.make.checkerboard node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Make Checkerboard Image",
					  "keywords" : [ "backdrop", "background", "checkers", "chess", "debug", "troubleshoot", "uvw", "mosaic" ],
					  "version" : "1.0.2",
					  "node": {
						  "exampleCompositions" : [ "MoveCheckerboardCenter.vuo" ]
					  }
				 });

static const char * checkerboardFragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	varying vec4 fragmentTextureCoordinate;

	uniform vec2 size;
	uniform vec4 color1;
	uniform vec4 color2;
	uniform vec2 center;
	uniform vec2 imageSize;

	void main()
	{
		// Based on the Gritz/Baldwin antialiased checkerboard shader.

		vec2 filterWidth = fwidth(fragmentTextureCoordinate.xy) / size;

		vec2 checkPos = fract((fragmentTextureCoordinate.xy - center) / size);
		vec2 p = smoothstep(vec2(0.5), filterWidth + vec2(0.5), checkPos) +
			(1 - smoothstep(vec2(0),   filterWidth,             checkPos));

		// Premultiply colors before blending, so a transparent color doesn't unduly influence an opaque color.
		float amount = p.x*p.y + (1-p.x)*(1-p.y);
		gl_FragColor = vec4(mix(color1.rgb*color1.a, color2.rgb*color2.a, amount),
							mix(color1.a,            color2.a,            amount));
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

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	instance->shader = VuoShader_make("Checkerboard Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, checkerboardFragmentShaderSource);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) upperLeftColor,
		VuoInputData(VuoColor,{"default":{"r":0,"g":0,"b":0,"a":1}}) upperRightColor,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.01}) squareSize,
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoInteger, {"default":640, "suggestedMin":1, "suggestedStep":32}) width,
		VuoInputData(VuoInteger, {"default":480, "suggestedMin":1, "suggestedStep":32}) height,
		VuoOutputData(VuoImage) image
)
{
	double aspect = (float)width / (float)height;
	double clampedSquareSize = MAX(squareSize, .001);
	VuoPoint2d cen = { (center.x+1)/2., (center.y*aspect + 1)/2. };

	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "size", VuoPoint2d_make(clampedSquareSize, clampedSquareSize * aspect));
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center", cen);
	VuoShader_setUniform_VuoColor  ((*instance)->shader, "color1", upperLeftColor);
	VuoShader_setUniform_VuoColor  ((*instance)->shader, "color2", upperRightColor);

	// Render.
	*image = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, width, height, VuoImageColorDepth_8);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
