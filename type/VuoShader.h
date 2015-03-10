/**
 * @file
 * VuoShader C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOSHADER_H
#define VUOSHADER_H

#include "VuoColor.h"
#include "VuoImage.h"
#include "VuoInteger.h"
#include "VuoPoint2d.h"
#include "VuoPoint3d.h"
#include "VuoPoint4d.h"
#include "VuoReal.h"
#include "VuoGlContext.h"
#include "VuoList_VuoInteger.h"
#include "VuoList_VuoImage.h"
#include "VuoList_VuoColor.h"

#include <dispatch/dispatch.h>

/**
 * @ingroup VuoTypes
 * @defgroup VuoShader VuoShader
 * A graphics shader program, specifying how to render a 3D object.
 *
 * @{
 */

/**
 * A macro to facilitate defining a GLSL shader in a C source file.
 */
#define VUOSHADER_GLSL_SOURCE(version,source) "#version " #version "\n" #source

/**
 * A graphics shader program, specifying how to render a 3D object.
 *
 * The struct is typedef'd to a pointer so that VuoShaders are reference-counted,
 * enabling us to automatically delete the GL Program Objects when the last reference is released.
 */
typedef struct _VuoShader
{
	char *summary; ///< Text describing the shader, displayed in port popovers.

	unsigned int glVertexShaderName;
	unsigned int glFragmentShaderName;
	unsigned int glProgramName;

	// The following pair of lists are used together to assign textures to texture units, then to assign texture unit numbers to shader uniforms.
	VuoList_VuoImage textures;
	VuoList_VuoInteger glTextureUniformLocations;

	dispatch_semaphore_t lock;	///< Serializes operations that modify the state of this GL program object.
} *VuoShader;

VuoShader VuoShader_make(const char *summary, const char *vertexShaderSource, const char *fragmentShaderSource);
char * VuoShader_summaryFromValue(const VuoShader value);
VuoShader VuoShader_valueFromJson(struct json_object * js);
struct json_object * VuoShader_jsonFromValue(const VuoShader value);
void VuoShader_setUniformFloat(VuoShader shader, VuoGlContext glContext, const char *uniformIdentifier, float value);
void VuoShader_setUniformPoint2d(VuoShader shader, VuoGlContext glContext, const char *uniformIdentifier, VuoPoint2d value);
void VuoShader_setUniformPoint3d(VuoShader shader, VuoGlContext glContext, const char *uniformIdentifier, VuoPoint3d value);
void VuoShader_setUniformPoint4d(VuoShader shader, VuoGlContext glContext, const char *uniformIdentifier, VuoPoint4d value);
void VuoShader_setUniformFloatArray(VuoShader shader, VuoGlContext glContext, const char *uniformIdentifier, const float* value, int length);
const char * VuoShader_getDefaultVertexShader(void);
VuoPoint2d VuoShader_samplerCoordinatesFromVuoCoordinates(VuoPoint2d vuoCoordinates, VuoImage image);
VuoReal VuoShader_samplerSizeFromVuoSize(VuoReal vuoSize);

VuoShader VuoShader_makeImageShader(void);
void VuoShader_resetTextures(VuoShader shader);
void VuoShader_addTexture(VuoShader shader, VuoGlContext glContext, const char *uniformIdentifier, VuoImage texture);
void VuoShader_activateTextures(VuoShader shader, VuoGlContext glContext);
void VuoShader_deactivateTextures(VuoShader shader, VuoGlContext glContext);

VuoShader VuoShader_makeColorShader(VuoColor color);
VuoShader VuoShader_makeLitColorShader(VuoColor diffuseColor, VuoColor highlightColor, VuoReal shininess);
VuoShader VuoShader_makeLitImageShader(VuoImage image, VuoReal alpha, VuoColor highlightColor, VuoReal shininess);
VuoShader VuoShader_makeLitImageDetailsShader(VuoImage image, VuoReal alpha, VuoImage specularImage, VuoImage normalImage);

VuoShader VuoShader_makeLinearGradientShader(VuoList_VuoColor colors, VuoPoint2d start, VuoPoint2d end);
VuoShader VuoShader_makeRadialGradientShader(VuoList_VuoColor colors, VuoPoint2d center, VuoReal radius, VuoReal width, VuoReal height);


/**
 *	Provides a GLSL method which converts rgb to hsl.  Accepts a vec3 and returns a vec3 (hsl).	
 */
#define VUOSHADER_GLSL_FRAGMENT_COLOR_CONVERSION_SOURCE "							\
	vec3 RgbToHsl(vec3 color)														\
	{																				\
		vec3 hsl;																	\
																					\
		float fmin = min(min(color.r, color.g), color.b);							\
		float fmax = max(max(color.r, color.g), color.b);							\
		float delta = fmax - fmin;													\
																					\
		hsl.z = (fmax + fmin) / 2.0;												\
																					\
		if (delta == 0.0)															\
		{																			\
			hsl.x = 0.0;															\
			hsl.y = 0.0;															\
		}																			\
		else																		\
		{																			\
			if (hsl.z < 0.5)														\
				hsl.y = delta / (fmax + fmin);										\
			else																	\
				hsl.y = delta / (2.0 - fmax - fmin);								\
																					\
			float deltaR = (((fmax - color.r) / 6.0) + (delta / 2.0)) / delta;		\
			float deltaG = (((fmax - color.g) / 6.0) + (delta / 2.0)) / delta;		\
			float deltaB = (((fmax - color.b) / 6.0) + (delta / 2.0)) / delta;		\
																					\
			if (color.r == fmax )													\
				hsl.x = deltaB - deltaG;											\
			else if (color.g == fmax)												\
				hsl.x = (1.0 / 3.0) + deltaR - deltaB;								\
			else if (color.b == fmax)												\
				hsl.x = (2.0 / 3.0) + deltaG - deltaR;								\
																					\
			if (hsl.x < 0.0)														\
				hsl.x += 1.0;														\
			else if (hsl.x > 1.0)													\
				hsl.x -= 1.0;														\
		}																			\
																					\
		return hsl;																	\
	}																				\
																					\
	float HueToRGB(float f1, float f2, float hue)									\
	{																				\
		if (hue < 0.0)																\
			hue += 1.0;																\
		else if (hue > 1.0)															\
			hue -= 1.0;																\
		float res;																	\
		if ((6.0 * hue) < 1.0)														\
			res = f1 + (f2 - f1) * 6.0 * hue;										\
		else if ((2.0 * hue) < 1.0)													\
			res = f2;																\
		else if ((3.0 * hue) < 2.0)													\
			res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;						\
		else																		\
			res = f1;																\
		return res;																	\
	}																				\
																					\
	vec3 HslToRgb(vec3 hsl)															\
	{																				\
		vec3 rgb;																	\
																					\
		if (hsl.y == 0.0)															\
			rgb = vec3(hsl.z);														\
		else																		\
		{																			\
			float f2;																\
																					\
			if (hsl.z < 0.5)														\
				f2 = hsl.z * (1.0 + hsl.y);											\
			else																	\
				f2 = (hsl.z + hsl.y) - (hsl.y * hsl.z);								\
																					\
			float f1 = 2.0 * hsl.z - f2;											\
																					\
			rgb.r = HueToRGB(f1, f2, hsl.x + (1.0/3.0));							\
			rgb.g = HueToRGB(f1, f2, hsl.x);										\
			rgb.b = HueToRGB(f1, f2, hsl.x - (1.0/3.0));							\
		}																			\
																					\
		return rgb;																	\
	}																				\
"

/**
 *	Provides the template for a glsl shader with hsl to rgb conversion methods defined (RgbToHsl(vec3 rgb), HslToRgb(vec3 hs))
 */
#define VUOSHADER_GLSL_FRAGMENT_SOURCE_WITH_COLOR_CONVERSIONS(source) "#version 120\n" VUOSHADER_GLSL_FRAGMENT_COLOR_CONVERSION_SOURCE "\n" #source


/// @{
/**
 * Automatically generated function.
 */
VuoShader VuoShader_valueFromString(const char *str);
char * VuoShader_stringFromValue(const VuoShader value);
/// @}

/**
 * @}
 */

#endif
