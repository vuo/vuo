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
#include "VuoReal.h"
#include "VuoGlContext.h"
#include "VuoList_VuoInteger.h"
#include "VuoList_VuoImage.h"

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
const char * VuoShader_getDefaultVertexShader(void);
VuoPoint2d VuoShader_samplerCoordinatesFromVuoCoordinates(VuoPoint2d vuoCoordinates, VuoImage image);
VuoReal VuoShader_samplerSizeFromVuoSize(VuoReal vuoSize);

VuoShader VuoShader_makeImageShader(void);
void VuoShader_resetTextures(VuoShader shader);
void VuoShader_addTexture(VuoShader shader, VuoGlContext glContext, const char *uniformIdentifier, VuoImage texture);
void VuoShader_activateTextures(VuoShader shader, VuoGlContext glContext);
void VuoShader_deactivateTextures(VuoShader shader, VuoGlContext glContext);

VuoShader VuoShader_makeColorShader(VuoColor color);

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
