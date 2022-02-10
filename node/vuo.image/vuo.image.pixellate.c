/**
 * @file
 * vuo.image.pixellate node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoPixelShape.h"

VuoModuleMetadata({
					 "title" : "Pixellate Image",
					 "keywords" : [
						 "pixels", "lofi", "simplify", "mosaic", "censor", "tile", "grid",
						 "rectangle", "linear", "cube", "square", "box", "overenlarge",
						 "triangle", "equilateral", "tangram", "PETSCII", "trixel",
						 "hexagon", "hexel", "honeycomb", "Settlers of Catan",
						 "pixelate", // American spelling
						 "filter"
					 ],
					 "version" : "1.2.0",
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

static const char * rectangleFragmentShader = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	varying vec2 fragmentTextureCoordinate;

	uniform sampler2D texture;
	uniform vec2 viewportSize;
	uniform vec2 pixelSize;
	uniform vec2 center;

	void main(void)
	{
		vec2 pos = fragmentTextureCoordinate;

		// Quantize the texture coordinate so it lands exactly on an output pixel,
		// since mod()ding near a big pixel boundary
		// can exaggerate the GPU's imprecise floating point math.
		pos = vec2(int(pos.x*viewportSize.x)/viewportSize.x,
				   int(pos.y*viewportSize.y)/viewportSize.y);

		vec2 centerOffset = mod(center - pixelSize/2., pixelSize);
		vec2 distanceFromCorner = mod(pos - centerOffset, pixelSize);
		pos = pos - distanceFromCorner + pixelSize/2.;
		gl_FragColor = VuoGlsl_sample(texture, pos);
	}
);


static const char * triangleFragmentShader = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	varying vec2 fragmentTextureCoordinate;

	uniform vec2 viewportSize;
	uniform float aspectRatio;

	uniform sampler2D texture;
	uniform vec2 pixelSize;
	uniform vec2 center;

	vec2 transform(vec2 textureCoordinate)
	{
		vec2 ar = vec2(1., aspectRatio);

		// Normalize
		textureCoordinate -= center;
		textureCoordinate /= ar;

		// Quantize to triangle grid
		{
			float triangleSize = pixelSize.x;
			vec2 tc = textureCoordinate / triangleSize;

			// Flip every other row
			float triY = fract(tc.y);
			if (mod(floor(tc.y),2.) > 0.)
				triY = 1. - triY;

			// Snap to the middle of the triangle row
			float triX = fract(tc.x);

			vec2 snapped;
			snapped.y = (floor(tc.y) + .5) * triangleSize;

			float dx = abs(triX - .5) * 2.;
			if (dx > 1. - triY || triX == 0.0)
			{
			   if (triX > .5)
				  snapped.x = (floor(tc.x) + 1.) * triangleSize;
			   else
				  snapped.x = floor(tc.x) * triangleSize;
			}
			else
			   snapped.x = (floor(tc.x) + .5) * triangleSize;

			textureCoordinate = snapped;
		}

		// Denormalize
		textureCoordinate *= ar;
		textureCoordinate += center;

		return textureCoordinate;
	}

	void main()
	{
		vec2 s = .25/viewportSize;
		vec2 sx = vec2(s.x, 0.);
		vec2 sy = vec2(0., s.y);
		vec4 a = VuoGlsl_sample(texture, transform(fragmentTextureCoordinate - sx));
		vec4 b = VuoGlsl_sample(texture, transform(fragmentTextureCoordinate + sx));
		vec4 c = VuoGlsl_sample(texture, transform(fragmentTextureCoordinate - sy));
		vec4 d = VuoGlsl_sample(texture, transform(fragmentTextureCoordinate + sy));
		gl_FragColor = (a+b+c+d)/4.;
	}
);

static const char * hexagonFragmentShader = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	varying vec2 fragmentTextureCoordinate;

	uniform vec2 viewportSize;
	uniform float aspectRatio;

	uniform sampler2D texture;
	uniform vec2 pixelSize;
	uniform vec2 center;

	float hexagonWidth4 = pixelSize.x/2.;
	float H = sqrt(3.) * hexagonWidth4;
	float S = ((3./2.) * H/sqrt(3.));

	vec2 hexCoord(ivec2 hexIndex)
	{
		int i = hexIndex.x;
		int j = hexIndex.y;
		vec2 r;
		r.x = float(i) * S;
		r.y = float(j) * H + mod(float(i),2.) * H/2.;
		return r;
	}

	ivec2 hexIndex(vec2 coord)
	{
		ivec2 r;
		float x = coord.x;
		float y = coord.y;
		int it = int(floor(x/S));
		float yts = y - mod(float(it),float(2)) * H/2.;
		int jt = int(floor((1./H) * yts));
		float xt = x - float(it) * S;
		float yt = yts - float(jt) * H;
		int deltaj = (yt > H/2.)? 1:0;
		float fcond = S * (2./3.) * abs(0.5 - yt/H);

		if (xt > fcond)
		{
			r.x = it;
			r.y = jt;
		}
		else
		{
			r.x = it - 1;
			r.y = jt - int(mod(float(r.x), 2.)) + deltaj;
		}

		return r;
	}

	vec2 transform(vec2 textureCoordinate)
	{
		vec2 c = center + vec2(hexagonWidth4/2., 0.);
		vec2 ar = vec2(1., aspectRatio);

		// Normalize
		textureCoordinate -= c;
		textureCoordinate /= ar;

		// Quantize to hex grid
		ivec2 hexIx = hexIndex(textureCoordinate);
		textureCoordinate = hexCoord(hexIx);

		// Denormalize
		textureCoordinate *= ar;
		textureCoordinate += c;

		textureCoordinate += vec2(hexagonWidth4) * ar;

		return textureCoordinate;
	}

	void main()
	{
		vec2 s = .25/viewportSize;
		vec2 sx = vec2(s.x, 0.);
		vec2 sy = vec2(0., s.y);
		vec4 a = VuoGlsl_sample(texture, transform(fragmentTextureCoordinate - sx));
		vec4 b = VuoGlsl_sample(texture, transform(fragmentTextureCoordinate + sx));
		vec4 c = VuoGlsl_sample(texture, transform(fragmentTextureCoordinate - sy));
		vec4 d = VuoGlsl_sample(texture, transform(fragmentTextureCoordinate + sy));
		gl_FragColor = (a+b+c+d)/4.;
	}
);


struct nodeInstanceData
{
	VuoShader shader;
	VuoPixelShape shape;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = NULL;
	instance->shape = -1;

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoPixelShape, {"default":"rectangle"}) pixelShape,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0, "suggestedMax":2, "suggestedStep":0.05, "name":"Pixel Width"}) pixelSize,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoOutputData(VuoImage) pixellatedImage
)
{
	if (!image)
	{
		*pixellatedImage = NULL;
		return;
	}

	if (!(*instance)->shader
	 || pixelShape != (*instance)->shape)
	{
		VuoRelease((*instance)->shader);

		if (pixelShape == VuoPixelShape_Rectangle)
		{
			(*instance)->shader = VuoShader_make("Pixellate Image (Rectangle)");
			VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, NULL, NULL, rectangleFragmentShader);
		}
		else if (pixelShape == VuoPixelShape_Triangle)
		{
			(*instance)->shader = VuoShader_make("Pixellate Image (Triangle)");
			VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, NULL, NULL, triangleFragmentShader);
		}
		else if (pixelShape == VuoPixelShape_Hexagon)
		{
			(*instance)->shader = VuoShader_make("Pixellate Image (Hexagon)");
			VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, NULL, NULL, hexagonFragmentShader);
		}

		VuoRetain((*instance)->shader);

		(*instance)->shape = pixelShape;
	}

	int w = image->pixelsWide, h = image->pixelsHigh;

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoPoint2d pixelSize2d = VuoPoint2d_multiply(VuoPoint2d_make(1., (float)w/h), VuoShader_samplerSizeFromVuoSize(VuoReal_makeNonzero(MAX(pixelSize, 0.))));
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "pixelSize",  pixelSize2d);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center", VuoShader_samplerCoordinatesFromVuoCoordinates(center, image));

	*pixellatedImage = VuoImageRenderer_render((*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
