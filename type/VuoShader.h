/**
 * @file
 * VuoShader C type definition.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoColor.h"
#include "VuoHeap.h"
#include "VuoImage.h"
#include "VuoInteger.h"
#include "VuoMesh.h"
#include "VuoPoint2d.h"
#include "VuoPoint3d.h"
#include "VuoPoint4d.h"
#include "VuoReal.h"
#include "VuoText.h"
#include "VuoGlContext.h"
#include "VuoList_VuoInteger.h"
#include "VuoList_VuoImage.h"
#include "VuoList_VuoColor.h"
#include "VuoList_VuoPoint4d.h"
#include "VuoList_VuoText.h"
#include "VuoGlPool.h"

#ifdef __cplusplus
class VuoShaderFile; ///< See @ref VuoShaderFile
#else
typedef void *VuoShaderFile; ///< See @ref VuoShaderFile
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoShader VuoShader
 * A graphics shader program, specifying how to render a 3D object.
 *
 * A VuoShader can contain up to 3 separate GL Program Objects,
 * each of which processes a different type of input primitive (points, lines, triangles).
 *
 * Each GL Program Object must contain a vertex shader, and can optionally contain a geometry and/or fragment shader.
 *
 * If no fragment shader is present, the program object is assumed to be used for Transform Feedback (see @ref VuoSceneObjectRenderer).
 *
 * The struct is typedef'd to a pointer so that VuoShaders are reference-counted,
 * enabling Vuo to automatically delete the GL Program Objects when the last reference is released.
 *
 * ## Usage
 *
 * To create a shader that supports rendering points, lines, and triangles to a framebuffer:
 * @code
 * VuoShader shader = VuoShader_make("Color Shader (Unlit)");
 *
 * VuoShader_addSource(shader, VuoMesh_Points, vertexShader1, geometryShader1, fragmentShader1);
 * VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_Points, 2);
 *
 * VuoShader_addSource(shader, VuoMesh_IndividualLines, vertexShader2, geometryShader2, fragmentShader2);
 * VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualLines, 2);
 *
 * VuoShader_addSource(shader, VuoMesh_IndividualTriangles, vertexShader3, NULL, fragmentShader3);
 *
 * VuoShader_setUniform_VuoColor(shader, "color", color);
 * @endcode
 *
 * To render using that shader:
 * @code
 * GLint positionAttribute, normalAttribute, textureCoordinateAttribute, colorAttribute;
 * VuoShader_getAttributeLocations(shader, elementAssemblyMethod, glContext, &positionAttribute, &normalAttribute, &textureCoordinateAttribute, &colorAttribute);
 *
 * // [...] enable vertex attribute arrays
 *
 * VuoShader_activate(shader, elementAssemblyMethod, glContext);
 *
 * // [...] glDrawArrays() or glDrawElements()
 *
 * VuoShader_deactivate(shader, elementAssemblyMethod, glContext);
 *
 * // [...] disable vertex attribute arrays
 * @endcode
 *
 * @{
 */

/**
 * References to shader source code and shader code uploaded to the GPU.
 */
typedef struct
{
	VuoText vertexSource;
	unsigned int glVertexShaderName;

	VuoText geometrySource;
	unsigned int glGeometryShaderName;
	unsigned int expectedOutputPrimitiveCount;
	bool mayChangeOutputPrimitiveCount;

	VuoText fragmentSource;
	unsigned int glFragmentShaderName;

	VuoGlProgram program;

	VuoBoolean compilationAttempted;  ///< If true, we've already attempted to compile this subshader.
} VuoSubshader;

/**
 * Holds values to eventually be assigned to a GL Program Object's uniforms.
 *
 * @version200Changed{Added list and matrix `value`s, and `compiledTextureTarget`.}
 */
typedef struct
{
	VuoText name;
	VuoText type;
	union
	{
		VuoImage image;
		VuoBoolean boolean;
		VuoInteger integer;
		VuoReal real;
		VuoPoint2d point2d;
		VuoPoint3d point3d;
		VuoPoint4d point4d;
		VuoColor color;
		VuoList_VuoBoolean booleans;
		VuoList_VuoInteger integers;
		VuoList_VuoReal reals;
		VuoList_VuoPoint2d point2ds;
		VuoList_VuoPoint3d point3ds;
		VuoList_VuoPoint4d point4ds;
		VuoList_VuoColor colors;
		float *mat2;
		float *mat3;
		float *mat4;
	} value;

	/**
	 * For uniforms of type Image, this holds the OpenGL texture target used when expanding the source code.
	 *
	 * This is set during VuoShader_ensureUploaded (not during VuoShader_setUniform_VuoImage).
	 */
	unsigned int compiledTextureTarget;
} VuoShaderUniform;

/**
 * A graphics shader program, specifying how to render a 3D object or a 2D image.
 *
 * @version200Changed{Added `activationCount`, `lastActivationTime`.}
 */
typedef struct _VuoShader
{
	VuoText name; ///< Text describing the shader, displayed in port popovers.

	VuoSubshader pointProgram;
	VuoSubshader lineProgram;
	VuoSubshader triangleProgram;

	VuoShaderUniform *uniforms;
	unsigned int uniformsCount;

	float objectScale;	///< Typically 1.  If the shader draws an object, this specifies how large the object is relative to the quad onto which it's drawn (e.g., `VuoShader_makeUnlitCircleShader()` is 0.5 since the circle it draws is half the size of the quad).

	/**
	 * For 3D object shaders, set this to true if the shader is meant to be a transparent overlay.
	 * @ref VuoSceneRenderer will disable backface culling and depth buffer writing while rendering with this shader.
	 * In the fragment shader, use `gl_FrontFacing` to discard backfaces or treat them differently, if desired.
	 *
	 * For 2D image shaders, set this to true to force @ref VuoImageRenderer_render to use a texture with an alpha channel.
	 */
	bool isTransparent;

	/**
	 * When enabled, the fragment shader's output alpha value is converted
	 * into a percentage of sub-pixel samples to cover with the shader's output color.
	 *
	 * Since this also affects the depth buffer, it provides
	 * a cheap form of order-independent transparency
	 * ("cheap" because it results in only a few discrete levels of transparency:
	 * 3 levels for 2x multisampling, 5 for 4x, and 9 for 8x,
	 * and results in "screen door" dithering artifacts).
	 *
	 * This is only effective when multisampling is enabled (@see VuoSceneRenderer_renderToImage).
	 */
	bool useAlphaAsCoverage;

	VuoImage colorBuffer;	///< The renderbuffer color texture captured for this shader (if any).
	VuoImage depthBuffer;	///< The renderbuffer depth texture captured for this shader (if any).

	uint64_t activationCount; ///< How many times has this shader instance been activated?  (Roughly, how many frames have been rendered.)
	VuoReal lastActivationTime; ///< The time (VuoLogGetElapsedTime) at which the shader was most recently activated.

	void *lock;	///< `dispatch_semaphore_t` to serialize operations that modify the state of this GL program object.
} *VuoShader;


/// @name Creating shaders from GLSL source code
/// @{
VuoShader VuoShader_make(const char *name);
VuoShader VuoShader_makeFromFile(VuoShaderFile *shaderFile);
void VuoShader_addSource(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, const char *vertexShaderSource, const char *geometryShaderSource, const char *fragmentShaderSource);
void VuoShader_setExpectedOutputPrimitiveCount(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, const unsigned int expectedOutputPrimitiveCount);
void VuoShader_setMayChangeOutputPrimitiveCount(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, const bool mayChangeOutputPrimitiveCount);
void VuoShader_setTransparent(VuoShader shader, const bool isTransparent);

/// A macro to facilitate defining a GLSL shader in a C source file.
#define VUOSHADER_GLSL_SOURCE(version,source) "#version " #version "\n" #source
/// @}


/// @name Creating standard shaders
/// @{
VuoShader VuoShader_makeDefaultShader(void);
VuoShader VuoShader_makeUnlitImageShader(VuoImage image, VuoReal alpha);
VuoShader VuoShader_makeUnlitAlphaPassthruImageShader(VuoImage image, bool flipped);
VuoShader VuoShader_makeGlTextureRectangleShader(VuoImage image, VuoReal alpha);
VuoShader VuoShader_makeGlTextureRectangleAlphaPassthruShader(VuoImage image, bool flipped);

VuoShader VuoShader_makeUnlitColorShader(VuoColor color);
VuoShader VuoShader_makeUnlitCircleShader(VuoColor color, VuoReal sharpness);
VuoShader VuoShader_makeUnlitRoundedRectangleShader(VuoColor color, VuoReal sharpness, VuoReal roundness, VuoReal aspect);
VuoShader VuoShader_makeUnlitRoundedRectangleTrackShader(VuoColor background, VuoColor active, VuoReal sharpness, VuoReal roundness, VuoReal aspect, VuoBoolean isHorizontal, VuoReal value);
VuoShader VuoShader_makeUnlitCheckmarkShader(VuoColor color, VuoColor outline, float thickness);

VuoShader VuoShader_makeLitColorShader(VuoColor diffuseColor, VuoColor highlightColor, VuoReal shininess);
VuoShader VuoShader_makeLitImageShader(VuoImage image, VuoReal alpha, VuoColor highlightColor, VuoReal shininess);
VuoShader VuoShader_makeLitImageDetailsShader(VuoImage image, VuoReal alpha, VuoImage specularImage, VuoImage normalImage);

VuoShader VuoShader_makeLinearGradientShader(void);
void VuoShader_setLinearGradientShaderValues(VuoShader shader, VuoList_VuoColor colors, VuoPoint2d start, VuoPoint2d end, VuoReal aspect, VuoReal noiseAmount);

VuoShader VuoShader_makeRadialGradientShader(void);
void VuoShader_setRadialGradientShaderValues(VuoShader shader, VuoList_VuoColor colors, VuoPoint2d center, VuoReal radius, VuoReal width, VuoReal height, VuoReal noiseAmount);

VuoShader VuoShader_makeFrostedGlassShader(void);
void VuoShader_setFrostedGlassShaderValues(VuoShader shader, VuoColor color, VuoReal brightness, VuoPoint2d noisePosition, VuoReal noiseTime, VuoReal noiseAmount, VuoReal noiseScale, VuoReal chromaticAberration, VuoInteger levels, VuoReal roughness, VuoReal spacing, VuoInteger iterations, float aspectRatio);
/// @}


/// @name Using shaders
/// @{
bool VuoShader_isTransformFeedback(VuoShader shader);
unsigned int VuoShader_getExpectedOutputPrimitiveCount(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode);
bool VuoShader_getMayChangeOutputPrimitiveCount(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode);

bool VuoShader_upload(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, VuoGlContext glContext, void *outIssues) VuoWarnUnusedResult;
bool VuoShader_getAttributeLocations(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, VuoGlContext glContext, int *positionLocation, int *normalLocation, int *textureCoordinateLocation, int *colorLocation) VuoWarnUnusedResult;
bool VuoShader_activate(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, VuoGlContext glContext, VuoGlProgram *outputProgram) VuoWarnUnusedResult;
void VuoShader_deactivate(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, VuoGlContext glContext);
void VuoShader_resetContext(VuoGlContext glContext);

void VuoShader_setUniform_VuoImage  (VuoShader shader, const char *uniformIdentifier, const VuoImage   image);
void VuoShader_setUniform_VuoBoolean(VuoShader shader, const char *uniformIdentifier, const VuoBoolean boolean);
void VuoShader_setUniform_VuoInteger(VuoShader shader, const char *uniformIdentifier, const VuoInteger integer);
void VuoShader_setUniform_VuoReal   (VuoShader shader, const char *uniformIdentifier, const VuoReal    real);
void VuoShader_setUniform_VuoPoint2d(VuoShader shader, const char *uniformIdentifier, const VuoPoint2d point2d);
void VuoShader_setUniform_VuoPoint3d(VuoShader shader, const char *uniformIdentifier, const VuoPoint3d point3d);
void VuoShader_setUniform_VuoPoint4d(VuoShader shader, const char *uniformIdentifier, const VuoPoint4d point4d);
void VuoShader_setUniform_VuoColor  (VuoShader shader, const char *uniformIdentifier, const VuoColor   color);
void VuoShader_setUniform_VuoList_VuoBoolean(VuoShader shader, const char *uniformIdentifier, const VuoList_VuoBoolean booleans);
void VuoShader_setUniform_VuoList_VuoInteger(VuoShader shader, const char *uniformIdentifier, const VuoList_VuoInteger integers);
void VuoShader_setUniform_VuoList_VuoReal   (VuoShader shader, const char *uniformIdentifier, const VuoList_VuoReal    reals);
void VuoShader_setUniform_VuoList_VuoPoint2d(VuoShader shader, const char *uniformIdentifier, const VuoList_VuoPoint2d point2ds);
void VuoShader_setUniform_VuoList_VuoPoint3d(VuoShader shader, const char *uniformIdentifier, const VuoList_VuoPoint3d point3ds);
void VuoShader_setUniform_VuoList_VuoPoint4d(VuoShader shader, const char *uniformIdentifier, const VuoList_VuoPoint4d point4ds);
void VuoShader_setUniform_VuoList_VuoColor  (VuoShader shader, const char *uniformIdentifier, const VuoList_VuoColor   colors);
void VuoShader_setUniform_mat2              (VuoShader shader, const char *uniformIdentifier,       float             *mat2);
void VuoShader_setUniform_mat3              (VuoShader shader, const char *uniformIdentifier,       float             *mat3);
void VuoShader_setUniform_mat4              (VuoShader shader, const char *uniformIdentifier,       float             *mat4);

VuoShader VuoShader_make_VuoColor(VuoColor color);
VuoShader VuoShader_make_VuoShader(VuoShader shader);
VuoShader VuoShader_make_VuoImage(VuoImage image);

VuoImage VuoShader_getUniform_VuoImage(VuoShader shader, const char *uniformIdentifier);
VuoImage VuoShader_getFirstImage(VuoShader shader);

VuoPoint2d VuoShader_samplerCoordinatesFromVuoCoordinates(VuoPoint2d vuoCoordinates, VuoImage image);
VuoReal VuoShader_samplerSizeFromVuoSize(VuoReal vuoSize);
VuoPoint2d VuoShader_samplerRectCoordinatesFromNormalizedCoordinates(VuoPoint2d normalizedCoordinates, VuoInteger imageWidth, VuoInteger imageHeight);

bool VuoShader_isOpaque(VuoShader shader);
bool VuoShader_isPopulated(VuoShader shader);
/// @}


/// @name Summary, serialization, and reference counting
/// @{
char * VuoShader_getSummary(const VuoShader value);
VuoShader VuoShader_makeFromJson(struct json_object * js);
struct json_object * VuoShader_getJson(const VuoShader value);

/// This type has a _getInterprocessJson() function.
#define VuoShader_REQUIRES_INTERPROCESS_JSON
struct json_object * VuoShader_getInterprocessJson(const VuoShader value);

/// Automatically generated function.
VuoShader VuoShader_makeFromString(const char *str);
/// Automatically generated function.
char * VuoShader_getString(const VuoShader value);
/// Automatically generated function.
void VuoShader_retain(VuoShader value);
/// Automatically generated function.
void VuoShader_release(VuoShader value);
/// @}


/**
 * @}
 */
