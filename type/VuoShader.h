/**
 * @file
 * VuoShader C type definition.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOSHADER_H
#define VUOSHADER_H

#include "VuoImage.h"
#include "VuoPoint2d.h"
#include "VuoReal.h"

/// @{
typedef void * VuoList_VuoInteger;
#define VuoList_VuoInteger_TYPE_DEFINED

typedef void * VuoList_VuoImage;
#define VuoList_VuoImage_TYPE_DEFINED
/// @}

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
} *VuoShader;

VuoShader VuoShader_make(const char *summary, const char *vertexShaderSource, const char *fragmentShaderSource);
char * VuoShader_summaryFromValue(const VuoShader value);
VuoShader VuoShader_valueFromJson(struct json_object * js);
struct json_object * VuoShader_jsonFromValue(const VuoShader value);
void VuoShader_setUniformFloat(VuoShader shader, const char *uniformIdentifier, float value);
void VuoShader_setUniformPoint2d(VuoShader shader, const char *uniformIdentifier, VuoPoint2d value);
const char * VuoShader_getDefaultVertexShader(void);
VuoPoint2d VuoShader_samplerCoordinatesFromVuoCoordinates(VuoPoint2d vuoCoordinates, VuoImage image);
VuoReal VuoShader_samplerSizeFromVuoSize(VuoReal vuoSize);

VuoShader VuoShader_makeImageShader(void);
void VuoShader_resetTextures(VuoShader shader);
void VuoShader_addTexture(VuoShader shader, VuoImage texture, const char *uniformIdentifier);
void VuoShader_activateTextures(VuoShader shader);
void VuoShader_deactivateTextures(VuoShader shader);

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
