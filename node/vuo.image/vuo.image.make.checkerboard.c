/**
 * @file
 * vuo.image.make.checkerboard node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Make Checkerboard Image",
					  "keywords" : [ "backdrop", "background", "checkers", "chess", "debug", "uvw", "mosaic" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "CompareImageGenerators.vuo" ]
					  }
				 });

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

static const char * checkerboardFragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,

	varying vec4 fragmentTextureCoordinate;

	uniform float size;
	uniform vec4 color1;
	uniform vec4 color2;
	uniform vec2 center;
	uniform vec2 imageSize;

	void main()
	{
		int toggle = 0;

		// convert to pixels to avoid floating point modulus errors
		int px = int( (fragmentTextureCoordinate.x - center.x) * imageSize.x );
		int py = int( (fragmentTextureCoordinate.y - center.y) * imageSize.y );
		int ps = int(size * imageSize.x);

		// strange that mod(int, int) doesn't return an int
		int x = px - int(mod(px, ps));
		int y = py - int(mod(py, ps));

		toggle += (mod(x/ps, 2) > 0 ? 1 : 0);
		toggle += (mod(y/ps, 2) > 0 ? 1 : 0);

		gl_FragColor = (toggle == 1 ? color1 : color2);
	}
);

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) upperLeftColor,
		VuoInputData(VuoColor,{"default":{"r":0,"g":0,"b":0,"a":1}}) upperRightColor,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0, "suggestedMax":2, "suggestedStep":0.1}) squareSize,
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoInteger, {"default":640, "suggestedMin":1, "suggestedStep":32}) width,
		VuoInputData(VuoInteger, {"default":480, "suggestedMin":1, "suggestedStep":32}) height,
		VuoOutputData(VuoImage) image
)
{
	VuoShader shader = VuoShader_make("Checkerboard Shader");
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, checkerboardFragmentShaderSource);
	VuoRetain(shader);

	VuoPoint4d col1 = { upperLeftColor.r, upperLeftColor.g, upperLeftColor.b, upperLeftColor.a };
	VuoPoint4d col2 = { upperRightColor.r, upperRightColor.g, upperRightColor.b, upperRightColor.a };
	VuoPoint2d cen = { (center.x+1)/2., (center.y+1)/2. };

	VuoShader_setUniform_VuoReal   (shader, "size", MAX(squareSize/2, .0001) );
	VuoShader_setUniform_VuoPoint2d(shader, "center", cen);
	VuoShader_setUniform_VuoPoint2d(shader, "imageSize", (VuoPoint2d){width, height});
	VuoShader_setUniform_VuoPoint4d(shader, "color1", col1);
	VuoShader_setUniform_VuoPoint4d(shader, "color2", col2);

	// Render.
	*image = VuoImageRenderer_draw((*instance)->imageRenderer, shader, width, height, VuoImageColorDepth_8);

	VuoRelease(shader);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
