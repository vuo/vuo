/**
 * @file
 * vuo.image.stainedGlass node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Make Stained Glass Image",
					  "keywords" : [
						  "filter",
						  "voronoi", "cellular",

						  "pixellate", "pixels", "lofi", "simplify", "cube", "square", "filter", "overenlarge", "mosaic", "censor",
						  "pixelate" // American spelling
					  ],
					  "version" : "1.0.1",
					  "node": {
						  "exampleCompositions" : [ "MakeStainedGlassImage.vuo" ]
					  }
				 });

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"
	\n#include "VuoGlslRandom.glsl"
	\n#include "noise2D.glsl"

	varying vec2 fragmentTextureCoordinate;

	uniform sampler2D texture;
	uniform vec2 gridSpacing;
	uniform vec2 center;
	uniform float chaos;
	uniform vec4 borderColor;
	uniform float borderWidth;
	uniform vec2 viewportSize;
	uniform float aspectRatio;

	/**
	 * Returns the grid point nearest to `uv`.
	 */
	vec2 makeGridPoint(vec2 uv)
	{
		uv -= center;
		vec2 gridPoint = gridSpacing * vec2(
				float(int(uv.x / gridSpacing.x + 0.00001)),
				float(int(uv.y / gridSpacing.y + 0.00001)))
			+ gridSpacing/2;
		gridPoint += center;

		gridPoint += snoise2D2D((gridPoint-center)/gridSpacing*7919) * gridSpacing * chaos;

		return gridPoint;
	}

	void main(void)
	{
		vec2 uv = fragmentTextureCoordinate;

		// Quantize the texture coordinate so it lands exactly on an output pixel,
		// since snapping near a big pixel boundary
		// can exaggerate the GPU's imprecise floating point math.
		uv = vec2((floor(uv.x*viewportSize.x)+.25)/viewportSize.x,
				  (floor(uv.y*viewportSize.y)+.25)/viewportSize.y);

		const float searchWidth = 2.;
		float aspectSquared = aspectRatio*aspectRatio;

		vec2 closest = vec2(0);
		float minDistance = 999;
		float secondMinDistance = 999;
		for (float y = uv.y - gridSpacing.y*searchWidth; y < uv.y + gridSpacing.y*searchWidth; y += gridSpacing.y)
		for (float x = uv.x - gridSpacing.x*searchWidth; x < uv.x + gridSpacing.x*searchWidth; x += gridSpacing.x)
		{
			vec2 p = makeGridPoint(vec2(x,y));

			// Aspect-corrected distance(p, uv)^2
			float d = (uv.x-p.x)*(uv.x-p.x) + (uv.y-p.y)*(uv.y-p.y)/aspectSquared;

			if (d < minDistance)
			{
				secondMinDistance = minDistance;

				closest = p;
				minDistance = d;
			}
			else if (d < secondMinDistance && d > minDistance)
				secondMinDistance = d;
		}

		float fw = fwidth(uv.x);
		float border = smoothstep(borderWidth-fw, borderWidth, sqrt(secondMinDistance) - sqrt(minDistance));
		gl_FragColor = mix(borderColor, VuoGlsl_sample(texture, closest), border);
	}
);

struct nodeInstanceData
{
	VuoShader shader;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_make("Stained Glass Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0, "suggestedMax":0.2, "suggestedStep":0.01}) tileSize,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":0.7, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) chaos,
		VuoInputData(VuoColor, {"default":{"r":0,"g":0,"b":0,"a":1}}) borderColor,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.05}) borderWidth,
		VuoOutputData(VuoImage) stainedGlassImage
)
{
	if (!image)
	{
		*stainedGlassImage = NULL;
		return;
	}

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);

	int w = image->pixelsWide, h = image->pixelsHigh;
	float gss = VuoShader_samplerSizeFromVuoSize(VuoReal_makeNonzero(MAX(0, tileSize)));
	VuoPoint2d gridSpacing = VuoPoint2d_multiply(VuoPoint2d_make(1., (float)w/h), gss);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "gridSpacing", gridSpacing);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center",      VuoShader_samplerCoordinatesFromVuoCoordinates(center, image));

	double clampedChaos = VuoReal_clamp(chaos, -1, 1);
	// Avoid near-zero chaos values, which expose floating-point imprecision.
	if (fabs(clampedChaos) < 0.00001)
		clampedChaos = copysign(0.00001, clampedChaos);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "chaos",       clampedChaos);

	VuoShader_setUniform_VuoColor  ((*instance)->shader, "borderColor", borderColor);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "borderWidth", VuoReal_clamp(borderWidth, 0, 1) * gss*1.4);

	*stainedGlassImage = VuoImageRenderer_render((*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
