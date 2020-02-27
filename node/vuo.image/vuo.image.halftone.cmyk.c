/**
 * @file
 * vuo.image.halftone.cmyk node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoImageNoise.h"
#include "VuoGradientNoise.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					  "title" : "Make CMYK Halftone Image",
					  "keywords" : [
						  "filter",
						  "newspaper", "magazine", "printer", "press", "reprographic",
						  "screen", "dither",
						  "registration", "alignment", "separation", "move", "prism", "chromatic", "aberration", "shift",
					  ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoCurve",
					  ],
					  "node": {
						  "exampleCompositions" : [ "SimulatePrintedImage.vuo" ]
					  }
				 });

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	\n#include "noise2D.glsl"

	varying vec2 fragmentTextureCoordinate;

	uniform sampler2D texture;

	// Adapted from "Procedural Textures in GLSL" by Stefan Gustavson,
	// "OpenGL Insights", ISBN 978-1-4398-9376-0, CRC Press.

	uniform vec4 threshold;
	uniform float noiseAmount;
	uniform float scale;
	uniform float undercolorRemoval;
	uniform vec2 cOffset;
	uniform vec2 mOffset;
	uniform vec2 yOffset;
	uniform float aspectRatio;
	uniform vec2 center;
	uniform float crossfadeAmount;

	const float blackLevel = .7;

	// 'threshold ' is constant , 'value ' is smoothly varying
	float aastep(float threshold , float value)
	{
		float afwidth = 0.7 * length(vec2(dFdx(value), dFdy(value)));
		// GLSL's fwidth(value) is abs(dFdx(value)) + abs(dFdy(value))
		return smoothstep(threshold - afwidth, threshold + afwidth, value);
	}

	void main(void)
	{
		vec2 st = fragmentTextureCoordinate;

		float splotchNoiseScale = scale*4;

		vec3 gamma = vec3(.3);
		vec3 cColor = pow(texture2D(texture, st + cOffset).rgb, gamma);
		vec3 mColor = pow(texture2D(texture, st + mOffset).rgb, gamma);
		vec3 yColor = pow(texture2D(texture, st + yOffset).rgb, gamma);
		vec3 kColor = pow(texture2D(texture, st          ).rgb, gamma);

		st -= center;
		st.y /= aspectRatio;
		float n = 0.1   * snoise2D1D(st*splotchNoiseScale  )
				+ 0.05  * snoise2D1D(st*splotchNoiseScale*2);
				+ 0.025 * snoise2D1D(st*splotchNoiseScale*4); // Fractal noise, 3 octaves
		n *= noiseAmount;

		vec4 cmyk;


		// Convert to CMYK.

		// Cyan, with its black subtracted
		cColor = 1. - cColor;
		cmyk.x = cColor.x - min(cColor.x, min(cColor.y, cColor.z)) * undercolorRemoval;

		// Magenta, with its black subtracted
		mColor = 1. - mColor;
		cmyk.y = mColor.y - min(mColor.x, min(mColor.y, mColor.z)) * undercolorRemoval;

		// Yellow, with its black subtracted
		yColor = 1. - yColor;
		cmyk.z = yColor.z - min(yColor.x, min(yColor.y, yColor.z)) * undercolorRemoval;

		// Black
		kColor = 1. - kColor;
		cmyk.w = min(kColor.x, min(kColor.y, kColor.z));

		// Avoid chickenpox on solid white.
		cmyk -= 0.0001;


		// Generate screens, at 15/-15/0/45 degree angles.

		float sqrt2 = 1.414213;
		vec2 aspect = vec2(1.,1./aspectRatio);

		vec2 Cuv = scale * mat2(0.966, -0.259, 0.259, 0.966) * (st + cOffset*aspect);
		Cuv = fract(Cuv) - 0.5;
		float c = aastep(threshold.x, sqrt(cmyk.x) - sqrt2*length(Cuv) + n);

		vec2 Muv = scale * mat2(0.966, 0.259, -0.259, 0.966) * (st + mOffset*aspect);
		Muv = fract(Muv) - 0.5;
		float m = aastep(threshold.y, sqrt(cmyk.y) - sqrt2*length(Muv) + n);

		vec2 Yuv = scale * (st + yOffset*aspect); // 0 deg
		Yuv = fract(Yuv) - 0.5;
		float y = aastep(threshold.z, sqrt(cmyk.z) - sqrt2*length(Yuv) + n);

		vec2 Kuv = scale * mat2(0.707, -0.707, 0.707, 0.707) * (st);
		Kuv = fract(Kuv) - 0.5;
		float k = aastep(threshold.w, sqrt(cmyk.w) - sqrt2*length(Kuv) + n);


		// Mix screens.

		vec3 screened = 1.0 - vec3(c, m, y);
		screened = mix(screened, vec3(0.), blackLevel*k);

		cmyk -= (threshold + vec4(.2)) / vec4(2.6);
		vec3 unscreened = 1. - min(vec3(1.), cmyk.xyz*2.);
		unscreened = mix(unscreened, vec3(0.), min(1., cmyk.w*2.));

		gl_FragColor = vec4(mix(screened, unscreened, crossfadeAmount), 1.);
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

	instance->shader = VuoShader_make("CMYK Halftone Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,

		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1.0,"y":-1.0}, "suggestedMax":{"x":1.0,"y":1.0}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) scale,
		VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) cyanAmount,
		VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) magentaAmount,
		VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) yellowAmount,
		VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) blackAmount,
		VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) undercolorRemoval,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":5.0, "suggestedStep":0.5}) splotchiness,
		VuoInputData(VuoReal, {"default":0.01, "suggestedMin":0.0, "suggestedMax":0.1, "suggestedStep":0.01}) colorOffset,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0.0, "suggestedMax":360.0, "suggestedStep":15.0}) colorOffsetAngle,

		VuoOutputData(VuoImage) halftoneImage
)
{
	if (!image)
	{
		*halftoneImage = NULL;
		return;
	}

	int w = image->pixelsWide, h = image->pixelsHigh;

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);

	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center",      VuoShader_samplerCoordinatesFromVuoCoordinates(center, image));

	double clampedScale = VuoReal_makeNonzero(MAX(0,scale)/100.);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "scale",       1./clampedScale);

	VuoShader_setUniform_VuoPoint4d((*instance)->shader, "threshold",   (VuoPoint4d){
										VuoReal_lerp(-1.5, 1.1, 1 - VuoReal_clamp(cyanAmount,    0, 1)),
										VuoReal_lerp(-1.5, 1.1, 1 - VuoReal_clamp(magentaAmount, 0, 1)),
										VuoReal_lerp(-1.5, 1.1, 1 - VuoReal_clamp(yellowAmount,  0, 1)),
										VuoReal_lerp(-1.5, 1.1, 1 - VuoReal_clamp(blackAmount,   0, 1))});
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "undercolorRemoval", undercolorRemoval);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "noiseAmount", splotchiness);

	double coa = colorOffsetAngle * M_PI/180.;
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "cOffset", VuoPoint2d_multiply((VuoPoint2d){ cos(coa),             sin(coa)                 }, -colorOffset/2.));
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "mOffset", VuoPoint2d_multiply((VuoPoint2d){ cos(coa + 2.*M_PI/3), sin(coa + 2.*M_PI/3)*w/h }, -colorOffset/2.));
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "yOffset", VuoPoint2d_multiply((VuoPoint2d){ cos(coa + 4.*M_PI/3), sin(coa + 4.*M_PI/3)*w/h }, -colorOffset/2.));

	// When the pattern gets really small, crossfade to a non-patterned version, to hide the aliasing/moiré.
	double crossfadeAmount = VuoReal_curve(clampedScale * w, 1, 0, 4, VuoCurve_Quadratic, VuoCurveEasing_In, VuoLoopType_None);
	VuoShader_setUniform_VuoReal((*instance)->shader, "crossfadeAmount", crossfadeAmount);

	*halftoneImage = VuoImageRenderer_render((*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
