/**
 * @file
 * vuo.image.filter.blend node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Blend Images",
					 "description" :
						"<p>Blends two images into a single image.</p> \
						<p><ul> \
						<li>`background` — The background image (bottom layer) to blend.</li> \
						<li>`foreground` — The foreground image (top layer) to blend.</li> \
						<li>`blendMode` — The way that the images should be blended. \
						For information about blend modes, see the <a href=\"http://en.wikipedia.org/wiki/Blend_modes\">Wikipedia page on blend modes</a>, \
						the <a href=\"http://help.adobe.com/en_US/photoshop/cs/using/WSfd1234e1c4b69f30ea53e41001031ab64-77eba.html\">Photoshop blend mode descriptions</a>, \
						and the <a href=\"http://docs.gimp.org/en/gimp-concepts-layer-modes.html\">GIMP blend mode descriptions</a>.</li> \
						<li>`foregroundOpacity` — The opacity that the foreground image component should have in the blended image, ranging from 0 to 1.</li> \
						</ul></p> \
						<p>The resulting image uses the dimensions of the background image. \
						The foreground image is stretched to match the size of the background image.</p> \
						<p>Thanks to <a href=\"http://mouaif.wordpress.com\">Romain Dura</a> for the GLSL implementations of many blend modes.</p>",
					 "keywords" : [ "combine", "mix", "merge", "layer", "composite", "channel",
						"normal", "add", "alpha", "opacity", "transparent",
						"multiply", "darker", "linear burn", "color burn", "burn",
						"screen", "lighter", "linear dodge", "color dodge", "dodge",
						"overlay", "soft light", "hard light", "vivid light", "linear light", "pin light", "light", "hard mix",
						"difference", "exclusion", "subtract", "divide",
						"hue", "saturation", "color", "luminosity" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoImageRenderer"
					 ],
					 "node": {
						 "isInterface" : false
					 }
				 });


#define FilterSource "\
	uniform sampler2D background;\
	uniform sampler2D foreground;\
	uniform float foregroundOpacity;\
	varying vec4 fragmentTextureCoordinate;\
	\
	vec4 Desaturate(vec3 color, float Desaturation)\
	{\
		vec3 grayXfer = vec3(0.3, 0.59, 0.11);\
		vec3 gray = vec3(dot(grayXfer, color));\
		return vec4(mix(color, gray, Desaturation), 1.0);\
	}\
	\
	float BlendAddf(float base, float blend) { return min(base + blend, 1.0); }\
	float BlendLinearDodgef(float base, float blend) { return min(base + blend, 1.0); }\
	float BlendSubstractf(float base, float blend) { return max(base + blend - 1.0, 0.0); }\
	float BlendLinearBurnf(float base, float blend) { return max(base + blend - 1.0, 0.0); }\
	\
	float BlendLightenf(float base, float blend) { return max(blend, base); }\
	float BlendDarkenf(float base, float blend) { return min(blend, base); }\
	float BlendLinearLightf(float base, float blend) { return (blend < 0.5 ? BlendLinearBurnf(base, (2.0 * blend)) : BlendLinearDodgef(base, (2.0 * (blend - 0.5)))); }\
	\
	float BlendScreenf(float base, float blend) { return (1.0 - ((1.0 - base) * (1.0 - blend))); }\
	float BlendOverlayf(float base, float blend) { return (base < 0.5 ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0 - blend))); }\
	float BlendSoftLightf(float base, float blend) { return ((blend < 0.5) ? (2.0 * base * blend + base * base * (1.0 - 2.0 * blend)) : (sqrt(base) * (2.0 * blend - 1.0) + 2.0 * base * (1.0 - blend))); }\
	float BlendColorDodgef(float base, float blend) { return ((blend == 1.0) ? blend : min(base / (1.0 - blend), 1.0)); }\
	float BlendColorBurnf(float base, float blend) { return (blend == 0.0) ? blend : max((1.0 - ((1.0 - base) / blend)), 0.0); }\
	\
	float BlendVividLightf(float base, float blend) { return ((blend < 0.5) ? BlendColorBurnf(base, (2.0 * blend)) : BlendColorDodgef(base, (2.0 * (blend - 0.5)))); }\
	float BlendPinLightf(float base, float blend) { return ((blend < 0.5) ? BlendDarkenf(base, (2.0 * blend)) : BlendLightenf(base, (2.0 *(blend - 0.5)))); }\
	float BlendHardMixf(float base, float blend) { return ((BlendVividLightf(base, blend) < 0.5) ? 0.0 : 1.0); }\
	float BlendReflectf(float base, float blend) { return ((blend == 1.0) ? blend : min(base * base / (1.0 - blend), 1.0)); }\
	vec3 RGBToHSL(vec3 color)\
	{\
		vec3 hsl;\
		\
		float fmin = min(min(color.r, color.g), color.b);\
		float fmax = max(max(color.r, color.g), color.b);\
		float delta = fmax - fmin;\
\
		hsl.z = (fmax + fmin) / 2.0;\
\
		if (delta == 0.0)\
		{\
			hsl.x = 0.0;\
			hsl.y = 0.0;\
		}\
		else\
		{\
			if (hsl.z < 0.5)\
				hsl.y = delta / (fmax + fmin);\
			else\
				hsl.y = delta / (2.0 - fmax - fmin);\
			\
			float deltaR = (((fmax - color.r) / 6.0) + (delta / 2.0)) / delta;\
			float deltaG = (((fmax - color.g) / 6.0) + (delta / 2.0)) / delta;\
			float deltaB = (((fmax - color.b) / 6.0) + (delta / 2.0)) / delta;\
\
			if (color.r == fmax )\
				hsl.x = deltaB - deltaG;\
			else if (color.g == fmax)\
				hsl.x = (1.0 / 3.0) + deltaR - deltaB;\
			else if (color.b == fmax)\
				hsl.x = (2.0 / 3.0) + deltaG - deltaR;\
\
			if (hsl.x < 0.0)\
				hsl.x += 1.0;\
			else if (hsl.x > 1.0)\
				hsl.x -= 1.0;\
		}\
\
		return hsl;\
	}\
\
	float HueToRGB(float f1, float f2, float hue)\
	{\
		if (hue < 0.0)\
			hue += 1.0;\
		else if (hue > 1.0)\
			hue -= 1.0;\
		float res;\
		if ((6.0 * hue) < 1.0)\
			res = f1 + (f2 - f1) * 6.0 * hue;\
		else if ((2.0 * hue) < 1.0)\
			res = f2;\
		else if ((3.0 * hue) < 2.0)\
			res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;\
		else\
			res = f1;\
		return res;\
	}\
\
	vec3 HSLToRGB(vec3 hsl)\
	{\
		vec3 rgb;\
		\
		if (hsl.y == 0.0)\
			rgb = vec3(hsl.z);\
		else\
		{\
			float f2;\
			\
			if (hsl.z < 0.5)\
				f2 = hsl.z * (1.0 + hsl.y);\
			else\
				f2 = (hsl.z + hsl.y) - (hsl.y * hsl.z);\
				\
			float f1 = 2.0 * hsl.z - f2;\
			\
			rgb.r = HueToRGB(f1, f2, hsl.x + (1.0/3.0));\
			rgb.g = HueToRGB(f1, f2, hsl.x);\
			rgb.b= HueToRGB(f1, f2, hsl.x - (1.0/3.0));\
		}\
		\
		return rgb;\
	}\
	"

#define BLEND_FILTERS(source) "#version 120\n" FilterSource "\n" #source

static const char * fragmentShaderSource_blendNormal = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 result = vec3( mix(base.r, blend.r, blend.a),
							mix(base.g, blend.g, blend.a),
							mix(base.b, blend.b, blend.a) );

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}

);

static const char * fragmentShaderSource_blendMultiply = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 result = vec3( base.r * blend.r,
							base.g * blend.g,
							base.b * blend.b );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}

);

static const char * fragmentShaderSource_blendLinearDodge = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 result = vec3(	BlendLinearDodgef(base.r, blend.r),
							BlendLinearDodgef(base.g, blend.g),
							BlendLinearDodgef(base.b, blend.b));

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}

);

static const char * fragmentShaderSource_blendDarkerComponent = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 result = vec3( min(base.r, blend.r), min(base.g, blend.g), min(base.b, blend.b) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}

);

static const char * fragmentShaderSource_blendDarkerColor = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 base_lum = RGBToHSL(base.rgb);
		vec3 blend_lum = RGBToHSL(blend.rgb);

		vec3 result = base_lum.b < blend_lum.b ? base.rgb : blend.rgb;

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}

);

static const char * fragmentShaderSource_blendLighterComponent = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 result = vec3( max(base.r, blend.r), max(base.g, blend.g), max(base.b, blend.b) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendLighterColor = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 base_lum = RGBToHSL(base.rgb);
		vec3 blend_lum = RGBToHSL(blend.rgb);

		vec3 result = base_lum.b > blend_lum.b ? base.rgb : blend.rgb;

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendLinearBurn = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 result = vec3(	base.r + blend.r - 1.,
							base.g + blend.g - 1.,
							base.b + blend.b - 1. );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendColorBurn = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		// 1-(1-b)/f
		vec3 result = vec3(	1-min((1-base.r)/blend.r,1.),
							1-min((1-base.g)/blend.g,1.),
							1-min((1-base.b)/blend.b,1.) );


		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendColorDodge = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		// b/(1-f)
		vec3 result = vec3(	base.r / (1-blend.r),
							base.g / (1-blend.g),
							base.b / (1-blend.b) );


		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendScreen = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		// 1-(1-b)•(1-f)
		vec3 result = vec3(	1-(1-base.r) * (1-blend.r),
							1-(1-base.g) * (1-blend.g),
							1-(1-base.b) * (1-blend.b) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendOverlay = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		// (base < 0.5 ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0 - blend)))
		vec3 result = vec3(	(base.r < 0.5 ? (2.0 * base.r * blend.r) : (1.0 - 2.0 * (1.0 - base.r) * (1.0 - blend.r))),
							(base.g < 0.5 ? (2.0 * base.g * blend.g) : (1.0 - 2.0 * (1.0 - base.g) * (1.0 - blend.g))),
							(base.b < 0.5 ? (2.0 * base.b * blend.b) : (1.0 - 2.0 * (1.0 - base.b) * (1.0 - blend.b))) );


		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendSoftLight = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		// ((blend < 0.5) ? (2.0 * base * blend + base * base * (1.0 - 2.0 * blend)) : (sqrt(base) * (2.0 * blend - 1.0) + 2.0 * base * (1.0 - blend)))

		vec3 result = vec3(	((blend.r < 0.5) ? (2.0 * base.r * blend.r + base.r * base.r * (1.0 - 2.0 * blend.r)) : (sqrt(base.r) * (2.0 * blend.r - 1.0) + 2.0 * base.r * (1.0 - blend.r))),
							((blend.g < 0.5) ? (2.0 * base.g * blend.g + base.g * base.g * (1.0 - 2.0 * blend.g)) : (sqrt(base.g) * (2.0 * blend.g - 1.0) + 2.0 * base.g * (1.0 - blend.g))),
							((blend.b < 0.5) ? (2.0 * base.b * blend.b + base.b * base.b * (1.0 - 2.0 * blend.b)) : (sqrt(base.b) * (2.0 * blend.b - 1.0) + 2.0 * base.b * (1.0 - blend.b))) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendHardLight = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 result = vec3(	(blend.r < 0.5 ? (2.0 * blend.r * base.r) : (1.0 - 2.0 * (1.0 - blend.r) * (1.0 - base.r))),
							(blend.g < 0.5 ? (2.0 * blend.g * base.g) : (1.0 - 2.0 * (1.0 - blend.g) * (1.0 - base.g))),
							(blend.b < 0.5 ? (2.0 * blend.b * base.b) : (1.0 - 2.0 * (1.0 - blend.b) * (1.0 - base.b))) );


		result = mix(blend.rgb, result, base.a);
		result = mix(base.rgb, result, blend.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendVividLight = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		 vec3 result = vec3(((blend.r < 0.5) ? BlendColorBurnf(base.r, (2.0 * blend.r)) : BlendColorDodgef(base.r, (2.0 * (blend.r - 0.5)))),
							((blend.g < 0.5) ? BlendColorBurnf(base.g, (2.0 * blend.g)) : BlendColorDodgef(base.g, (2.0 * (blend.g - 0.5)))),
							((blend.b < 0.5) ? BlendColorBurnf(base.b, (2.0 * blend.b)) : BlendColorDodgef(base.b, (2.0 * (blend.b - 0.5)))) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendLinearLight = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		 vec3 result = vec3(BlendLinearLightf(base.r, blend.r),
							BlendLinearLightf(base.g, blend.g),
							BlendLinearLightf(base.b, blend.b) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendPinLight = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		 vec3 result = vec3(BlendPinLightf(base.r, blend.r),
							BlendPinLightf(base.g, blend.g),
							BlendPinLightf(base.b, blend.b) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendHardMix = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		 vec3 result = vec3(BlendHardMixf(base.r, blend.r),
							BlendHardMixf(base.g, blend.g),
							BlendHardMixf(base.b, blend.b) );


		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendDifference = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		 vec3 result = vec3(abs(base.r - blend.r),
							abs(base.g - blend.g),
							abs(base.b - blend.b) );


		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendExclusion = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		// b+f-2•b•f
		 vec3 result = vec3( base.r+blend.r - 2*base.r*blend.r,
							 base.g+blend.g - 2*base.g*blend.g,
							 base.b+blend.b - 2*base.b*blend.b );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendSubtract = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 result = vec3( clamp(base.r - blend.r, 0., 1.),
							clamp(base.g - blend.g, 0., 1.),
							clamp(base.b - blend.b, 0., 1.) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendDivide = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		 vec3 result = vec3( clamp(base.r / blend.r, 0., 1.),
							 clamp(base.g / blend.g, 0., 1.),
							 clamp(base.b / blend.b, 0., 1.) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendHue = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 baseHSL = RGBToHSL(base.rgb);
		vec3 result = HSLToRGB(vec3(RGBToHSL(blend.rgb).r, baseHSL.g, baseHSL.b));

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendSaturation = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 baseHSL = RGBToHSL(base.rgb);
		vec3 result = HSLToRGB(vec3(baseHSL.r, RGBToHSL(blend.rgb).g, baseHSL.b));

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendColor = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 blendHSL = RGBToHSL(blend.rgb);
		vec3 result = HSLToRGB(vec3(blendHSL.r, blendHSL.g, RGBToHSL(base.rgb).b));

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendLuminosity = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 baseHSL = RGBToHSL(base.rgb);
		vec3 result = HSLToRGB(vec3(baseHSL.r, baseHSL.g, RGBToHSL(blend.rgb).b));

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

struct nodeInstanceData
{
	VuoShader shader;
	VuoImageRenderer imageRenderer;
	VuoInteger currentBlendMode;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->currentBlendMode = -1;	// set this to a negative value on initialization, forcing shader init on first event.

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) background,
		VuoInputData(VuoImage) foreground,
		VuoInputData(VuoBlendMode, {"default":"normal"}) blendMode,
		VuoInputData(VuoReal, {"default":0.5,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) foregroundOpacity,
		VuoOutputData(VuoImage) blended
)
{
	if (!background || !foreground)
		return;

	// if the blend mode has changed or is uninitialized, init here
	if( blendMode != (*instance)->currentBlendMode )
	{
		// if this is the first time initializing, need to init the imagerenderer too
		if( (*instance)->currentBlendMode < 0 )
		{
			(*instance)->imageRenderer = VuoImageRenderer_make();
			VuoRetain((*instance)->imageRenderer);
		}

		const char *fragmentShaderSource = fragmentShaderSource_blendNormal;
		switch(blendMode)
		{
			case VuoBlendMode_Normal:
				fragmentShaderSource = fragmentShaderSource_blendNormal;
				break;
			case VuoBlendMode_Multiply:
				fragmentShaderSource = fragmentShaderSource_blendMultiply;
				break;
			case VuoBlendMode_DarkerComponent:
				fragmentShaderSource = fragmentShaderSource_blendDarkerComponent;
				break;
			case VuoBlendMode_DarkerColor:
				fragmentShaderSource = fragmentShaderSource_blendDarkerColor;
				break;
			case VuoBlendMode_LinearBurn:
				fragmentShaderSource = fragmentShaderSource_blendLinearBurn;
				break;
			case VuoBlendMode_ColorBurn:
				fragmentShaderSource = fragmentShaderSource_blendColorBurn;
				break;
			case VuoBlendMode_Screen:
				fragmentShaderSource = fragmentShaderSource_blendScreen;
				break;
			case VuoBlendMode_LighterComponent:
				fragmentShaderSource = fragmentShaderSource_blendLighterComponent;
				break;
			case VuoBlendMode_LighterColor:
				fragmentShaderSource = fragmentShaderSource_blendLighterColor;
				break;
			case VuoBlendMode_LinearDodge:
				fragmentShaderSource = fragmentShaderSource_blendLinearDodge;
				break;
			case VuoBlendMode_ColorDodge:
				fragmentShaderSource = fragmentShaderSource_blendColorDodge;
				break;
			case VuoBlendMode_Overlay:
				fragmentShaderSource = fragmentShaderSource_blendOverlay;
				break;
			case VuoBlendMode_SoftLight:
				fragmentShaderSource = fragmentShaderSource_blendSoftLight;
				break;
			case VuoBlendMode_HardLight:
				fragmentShaderSource = fragmentShaderSource_blendHardLight;
				break;
			case VuoBlendMode_VividLight:
				fragmentShaderSource = fragmentShaderSource_blendVividLight;
				break;
			case VuoBlendMode_LinearLight:
				fragmentShaderSource = fragmentShaderSource_blendLinearLight;
				break;
			case VuoBlendMode_PinLight:
				fragmentShaderSource = fragmentShaderSource_blendPinLight;
				break;
			case VuoBlendMode_HardMix:
				fragmentShaderSource = fragmentShaderSource_blendHardMix;
				break;
			case VuoBlendMode_Difference:
				fragmentShaderSource = fragmentShaderSource_blendDifference;
				break;
			case VuoBlendMode_Exclusion:
				fragmentShaderSource = fragmentShaderSource_blendExclusion;
				break;
			case VuoBlendMode_Subtract:
				fragmentShaderSource = fragmentShaderSource_blendSubtract;
				break;
			case VuoBlendMode_Divide:
				fragmentShaderSource = fragmentShaderSource_blendDivide;
				break;
			case VuoBlendMode_Hue:
				fragmentShaderSource = fragmentShaderSource_blendHue;
				break;
			case VuoBlendMode_Saturation:
				fragmentShaderSource = fragmentShaderSource_blendSaturation;
				break;
			case VuoBlendMode_Color:
				fragmentShaderSource = fragmentShaderSource_blendColor;
				break;
			case VuoBlendMode_Luminosity:
				fragmentShaderSource = fragmentShaderSource_blendLuminosity;
				break;
		}
		if( (*instance)->currentBlendMode >= 0 )
			VuoRelease((*instance)->shader);
		(*instance)->shader = VuoShader_make(VuoBlendMode_summaryFromValue(blendMode), VuoShader_getDefaultVertexShader(), fragmentShaderSource);
		VuoRetain((*instance)->shader);
		(*instance)->currentBlendMode = blendMode;
	}

	// Associate the input image with the shader.
	VuoShader_resetTextures((*instance)->shader);
	VuoShader_addTexture((*instance)->shader, background, "background");
	VuoShader_addTexture((*instance)->shader, foreground, "foreground");

	VuoShader_setUniformFloat((*instance)->shader, "foregroundOpacity", foregroundOpacity);

	// Render.
	*blended = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, background->pixelsWide, background->pixelsHigh);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	if( (*instance)->currentBlendMode >= 0 )
	{
		VuoRelease((*instance)->shader);
		VuoRelease((*instance)->imageRenderer);
	}
}
