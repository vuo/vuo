/**
 * @file
 * VuoSceneRenderer implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <list>
#include <map>
#include <string>

#include "VuoSceneRenderer.h"

#include "VuoCglPixelFormat.h"
#include "VuoSceneObject.h"
#include "VuoGlPool.h"
#include "VuoImageText.h"

#include <CoreServices/CoreServices.h>

#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>
/// @{
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
/// @}

#include <dispatch/dispatch.h>

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoSceneRenderer",
					 "dependencies" : [
						 "VuoBoolean",
						 "VuoImage",
						 "VuoImageColorDepth",
						 "VuoImageText",
						 "VuoSceneObject",
						 "VuoText",
						 "VuoList_VuoSceneObject",
						 "VuoGlContext"
					 ]
				 });
#endif
}

#ifdef PROFILE
typedef std::pair<std::string, double> VuoProfileEntry;	///< Description + time (seconds).
static bool VuoProfileSort(const VuoProfileEntry &first, const VuoProfileEntry &second)
{
	return first.second < second.second;
}
#endif

typedef std::pair<void *, void *> VuoSceneRendererMeshShader;	///< A VuoMesh/VuoShader combination
typedef std::map<VuoSceneRendererMeshShader, GLuint> VuoSceneRendererMeshShaderVAOs; ///< The VAO for each VuoMesh/VuoShader combination

/**
 * GL Objects corresponding with a VuoSceneObject instance.
 */
class VuoSceneRendererInternal_object
{
public:
	GLuint vao; ///< This VuoSceneObject's VAO.

	VuoMesh overrideMesh;	///< If non-null, this mesh is rendered instead of the actual object's mesh.
	enum
	{
		RealSize_Inherit,
		RealSize_True,
	} overrideIsRealSize;	///< If not Inherit, this overrides the actual object's isRealSize property.
	VuoShader overrideShader;	///< If non-null, this shader is rendered instead of the actual object's shader.

	unsigned long childObjectCount; ///< Size of `childObjects`
	VuoSceneRendererInternal_object *childObjects; ///< Hierarchy
};

/**
 * Internal state data for a VuoSceneRenderer instance.
 */
class VuoSceneRendererInternal
{
public:
	dispatch_semaphore_t scenegraphSemaphore; ///< Serializes access to other data in this structure.
	bool needToRegenerateProjectionMatrix;
	unsigned int viewportWidth;
	unsigned int viewportHeight;
	float backingScaleFactor;

	VuoSceneObject rootSceneObjectPending;			///< The latest scene from VuoSceneRenderer_setRootSceneObject().
	bool           rootSceneObjectPendingValid;		///< Whether rootSceneObjectPending has already been initialized.
	bool           rootSceneObjectPendingUpdated;	///< Whether rootSceneObjectPending contains a new scene (compared to rootSceneObject).

	VuoSceneObject rootSceneObject;					///< The latest scene that's actually been uploaded and drawn.
	bool           rootSceneObjectValid;			///< Whether rootSceneObject has already been initialized (and uploaded and drawn).
	VuoSceneRendererInternal_object rootSceneObjectInternal;

	VuoSceneRendererMeshShaderVAOs meshShaderItems;	///< Given a VuoMesh and a VuoShader, stores a set of VAOs.

	VuoShader vignetteShader;
	VuoSceneObject vignetteQuad;
	VuoSceneRendererInternal_object vignetteQuadInternal;

	float projectionMatrix[16]; ///< Column-major 4x4 matrix
	float cameraMatrixInverse[16]; ///< Column-major 4x4 matrix
	VuoText cameraName;
	VuoSceneObject camera;
	VuoBoolean useLeftCamera;

	VuoColor ambientColor;
	float ambientBrightness;
	VuoList_VuoSceneObject pointLights;
	VuoList_VuoSceneObject spotLights;

	GLint glContextRendererID;

	/**
	 * Store OpenGL state locally so we can change only when necessary,
	 * since calling glEnable/Disable marks the state dirty even if the state was unchanged.
	 */
	struct glState
	{
		bool vGL_DEPTH_MASK;
		bool vGL_DEPTH_TEST;

		bool vGL_CULL_FACE;
		GLenum vGL_CULL_FACE_MODE;

		GLenum vGL_BLEND_SRC_RGB;
		GLenum vGL_BLEND_DST_RGB;
		GLenum vGL_BLEND_SRC_ALPHA;
		GLenum vGL_BLEND_DST_ALPHA;

		GLenum vGL_BLEND_EQUATION_RGB;
		GLenum vGL_BLEND_EQUATION_ALPHA;

		bool vGL_SAMPLE_ALPHA_TO_COVERAGE;
		bool vGL_SAMPLE_ALPHA_TO_ONE;
	} glState;

#ifdef PROFILE
	std::list<VuoProfileEntry> profileTimes;	///< How long each object took to render.
#endif
};

/**
 * Ensures the specified OpenGL `cap`ability is set to `value` (avoiding a state change if possible).
 */
#define VuoSceneRenderer_setGL(cap, value) \
	do { \
		if (sceneRenderer->glState.v ## cap != value) \
		{ \
			if (value) \
				glEnable(cap); \
			else \
				glDisable(cap); \
			sceneRenderer->glState.v ## cap = value; \
		} \
	} while(0)

/**
 * Ensures the OpenGL depth mask is set to `value` (avoiding a state change if possible).
 */
#define VuoSceneRenderer_setGLDepthMask(value) \
	do { \
		if (sceneRenderer->glState.vGL_DEPTH_MASK != value) \
		{ \
			glDepthMask(value); \
			sceneRenderer->glState.vGL_DEPTH_MASK = value; \
		} \
	} while(0)

/**
 * Ensures the OpenGL face culling mode is set to `value` (avoiding a state change if possible).
 */
#define VuoSceneRenderer_setGLFaceCulling(value) \
	do { \
		if (sceneRenderer->glState.vGL_CULL_FACE_MODE != value) \
		{ \
			glCullFace(value); \
			sceneRenderer->glState.vGL_CULL_FACE_MODE = value; \
		} \
	} while(0)

/**
 * Ensures the OpenGL blend function is set to the specified values (avoiding a state change if possible).
 */
#define VuoSceneRenderer_setGLBlendFunction(srcRGB, dstRGB, srcAlpha, dstAlpha) \
	do { \
		if (sceneRenderer->glState.vGL_BLEND_SRC_RGB   != srcRGB    \
		 || sceneRenderer->glState.vGL_BLEND_DST_RGB   != dstRGB    \
		 || sceneRenderer->glState.vGL_BLEND_SRC_ALPHA != srcAlpha  \
		 || sceneRenderer->glState.vGL_BLEND_DST_ALPHA != dstAlpha) \
		{ \
			glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha); \
			sceneRenderer->glState.vGL_BLEND_SRC_RGB   = srcRGB;   \
			sceneRenderer->glState.vGL_BLEND_DST_RGB   = dstRGB;   \
			sceneRenderer->glState.vGL_BLEND_SRC_ALPHA = srcAlpha; \
			sceneRenderer->glState.vGL_BLEND_DST_ALPHA = dstAlpha; \
		} \
	} while(0)

/**
 * Ensures the OpenGL blend equation is set to the specified values (avoiding a state change if possible).
 */
#define VuoSceneRenderer_setGLBlendEquation(modeRGB, modeAlpha) \
	do { \
		if (sceneRenderer->glState.vGL_BLEND_EQUATION_RGB   != modeRGB    \
		 || sceneRenderer->glState.vGL_BLEND_EQUATION_ALPHA != modeAlpha) \
		{ \
			glBlendEquationSeparate(modeRGB, modeAlpha); \
			sceneRenderer->glState.vGL_BLEND_EQUATION_RGB = modeRGB; \
			sceneRenderer->glState.vGL_BLEND_EQUATION_ALPHA = modeAlpha; \
		} \
	} while(0)

void VuoSceneRenderer_destroy(VuoSceneRenderer sr);

static void VuoSceneRenderer_uploadSceneObject(VuoSceneRendererInternal *sceneRenderer, VuoSceneObject so, VuoSceneRendererInternal_object *soi, VuoGlContext glContext, bool cache);

#if LIBRARY_PREMIUM_AVAILABLE
#include "premium/VuoSceneRendererPremium.h"
#endif

/**
 * Creates a reference-counted object for rendering a scenegraph.
 *
 * @threadAny
 */
VuoSceneRenderer VuoSceneRenderer_make(float backingScaleFactor)
{
//	VDebugLog("backingScaleFactor=%g", backingScaleFactor);
	VuoSceneRendererInternal *sceneRenderer = new VuoSceneRendererInternal;
	VuoRegister(sceneRenderer, VuoSceneRenderer_destroy);

	sceneRenderer->scenegraphSemaphore = dispatch_semaphore_create(1);
	sceneRenderer->rootSceneObjectPendingValid = false;
	sceneRenderer->rootSceneObjectPendingUpdated = false;
	sceneRenderer->rootSceneObjectValid = false;
	sceneRenderer->needToRegenerateProjectionMatrix = false;
	sceneRenderer->cameraName = NULL;
	sceneRenderer->camera = VuoSceneObject_makeDefaultCamera();
	VuoSceneObject_retain(sceneRenderer->camera);
	sceneRenderer->backingScaleFactor = backingScaleFactor;
	sceneRenderer->viewportWidth = 0;
	sceneRenderer->viewportHeight = 0;

	// sceneRenderer->glState gets initialized in VuoSceneRenderer_draw.

	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
		sceneRenderer->vignetteShader = NULL;
#if LIBRARY_PREMIUM_AVAILABLE
		VuoSceneRendererPremium_init(sceneRenderer, cgl_ctx);
#endif

		CGLGetParameter(cgl_ctx, kCGLCPCurrentRendererID, &sceneRenderer->glContextRendererID);
	});

	return (VuoSceneRenderer)sceneRenderer;
}

static void VuoSceneRenderer_regenerateProjectionMatrixInternal(VuoSceneRendererInternal *sceneRenderer);

/**
 * Using the first camera found in the scene (or VuoSceneObject_makeDefaultCamera() if there is no camera in the scene),
 * recalculates the projection matrix based on the specified viewport @c width and @c height.
 *
 * @threadAny
 */
void VuoSceneRenderer_regenerateProjectionMatrix(VuoSceneRenderer sr, unsigned int width, unsigned int height)
{
//	VDebugLog("%dx%d", width, height);
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;

	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);

	// Store the viewport size (in case we need to regenerate the projection matrix later)
	sceneRenderer->viewportWidth = width;
	sceneRenderer->viewportHeight = height;

	VuoSceneRenderer_regenerateProjectionMatrixInternal(sceneRenderer);

	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);
}

/**
 * Helper for VuoSceneRenderer_regenerateProjectionMatrix and VuoSceneRenderer_draw.
 *
 * Must be called while scenegraphSemaphore is locked.
 */
void VuoSceneRenderer_regenerateProjectionMatrixInternal(VuoSceneRendererInternal *sceneRenderer)
{
	VuoSceneObject camera;

	{
		bool foundCamera = false;

		if (sceneRenderer->rootSceneObjectValid)
		{
			foundCamera = VuoSceneObject_findCamera(sceneRenderer->rootSceneObject, sceneRenderer->cameraName, &camera);
			if (!foundCamera && sceneRenderer->cameraName && strlen(sceneRenderer->cameraName) != 0)
				// Search again, and this time just pick the first camera we find.
				foundCamera = VuoSceneObject_findCamera(sceneRenderer->rootSceneObject, "", &camera);
		}

		if (!foundCamera)
			camera = VuoSceneObject_makeDefaultCamera();
	}


	// Build a projection matrix for a camera located at the origin, facing along the -z axis.
	float aspectRatio = (float)sceneRenderer->viewportWidth/(float)sceneRenderer->viewportHeight;
	if (camera.type == VuoSceneObjectSubType_PerspectiveCamera)
	{
		float halfFieldOfView = (camera.cameraFieldOfView * (float)M_PI/180.f) / 2.f;

		// left matrix column
		sceneRenderer->projectionMatrix[ 0] = 1.f/tanf(halfFieldOfView);
		sceneRenderer->projectionMatrix[ 1] = 0;
		sceneRenderer->projectionMatrix[ 2] = 0;
		sceneRenderer->projectionMatrix[ 3] = 0;

		sceneRenderer->projectionMatrix[ 4] = 0;
		sceneRenderer->projectionMatrix[ 5] = aspectRatio/tanf(halfFieldOfView);
		sceneRenderer->projectionMatrix[ 6] = 0;
		sceneRenderer->projectionMatrix[ 7] = 0;

		sceneRenderer->projectionMatrix[ 8] = 0;
		sceneRenderer->projectionMatrix[ 9] = 0;
		sceneRenderer->projectionMatrix[10] = (camera.cameraDistanceMax+camera.cameraDistanceMin)/(camera.cameraDistanceMin-camera.cameraDistanceMax);
		sceneRenderer->projectionMatrix[11] = -1.f;

		// right matrix column
		sceneRenderer->projectionMatrix[12] = 0;
		sceneRenderer->projectionMatrix[13] = 0;
		sceneRenderer->projectionMatrix[14] = 2.f*camera.cameraDistanceMax*camera.cameraDistanceMin/(camera.cameraDistanceMin-camera.cameraDistanceMax);
		sceneRenderer->projectionMatrix[15] = 0;
	}
	else if (camera.type == VuoSceneObjectSubType_StereoCamera)
	{
		float halfFieldOfView = (camera.cameraFieldOfView * (float)M_PI/180.f) / 2.f;
		float top = camera.cameraDistanceMin * tanf(halfFieldOfView);
		float right = aspectRatio*top;
		float frustumshift = (camera.cameraIntraocularDistance/2.f) * camera.cameraDistanceMin / camera.cameraConfocalDistance;
		if (!sceneRenderer->useLeftCamera)
			frustumshift *= -1.f;

		// column 0
		sceneRenderer->projectionMatrix[ 0] = 1.f/tanf(halfFieldOfView);
		sceneRenderer->projectionMatrix[ 1] = 0;
		sceneRenderer->projectionMatrix[ 2] = 0;
		sceneRenderer->projectionMatrix[ 3] = 0;

		// column 1
		sceneRenderer->projectionMatrix[ 4] = 0;
		sceneRenderer->projectionMatrix[ 5] = aspectRatio/tanf(halfFieldOfView);
		sceneRenderer->projectionMatrix[ 6] = 0;
		sceneRenderer->projectionMatrix[ 7] = 0;

		// column 2
		sceneRenderer->projectionMatrix[ 8] = 2.f * frustumshift / right;
		sceneRenderer->projectionMatrix[ 9] = 0;
		sceneRenderer->projectionMatrix[10] = (camera.cameraDistanceMax+camera.cameraDistanceMin)/(camera.cameraDistanceMin-camera.cameraDistanceMax);
		sceneRenderer->projectionMatrix[11] = -1.f;

		// column 3
		sceneRenderer->projectionMatrix[12] = (sceneRenderer->useLeftCamera? 1.f : -1.f) * camera.cameraIntraocularDistance/2.f;
		sceneRenderer->projectionMatrix[13] = 0;
		sceneRenderer->projectionMatrix[14] = 2.f*camera.cameraDistanceMax*camera.cameraDistanceMin/(camera.cameraDistanceMin-camera.cameraDistanceMax);
		sceneRenderer->projectionMatrix[15] = 0;
	}
	else if (camera.type == VuoSceneObjectSubType_OrthographicCamera)
	{
		float halfWidth = camera.cameraWidth / 2.f;

		// left matrix column
		sceneRenderer->projectionMatrix[ 0] = 1.f/halfWidth;
		sceneRenderer->projectionMatrix[ 1] = 0;
		sceneRenderer->projectionMatrix[ 2] = 0;
		sceneRenderer->projectionMatrix[ 3] = 0;

		sceneRenderer->projectionMatrix[ 4] = 0;
		sceneRenderer->projectionMatrix[ 5] = aspectRatio/halfWidth;
		sceneRenderer->projectionMatrix[ 6] = 0;
		sceneRenderer->projectionMatrix[ 7] = 0;

		sceneRenderer->projectionMatrix[ 8] = 0;
		sceneRenderer->projectionMatrix[ 9] = 0;
		sceneRenderer->projectionMatrix[10] = -2.f / (camera.cameraDistanceMax-camera.cameraDistanceMin);
		sceneRenderer->projectionMatrix[11] = 0;

		// right matrix column
		sceneRenderer->projectionMatrix[12] = 0;
		sceneRenderer->projectionMatrix[13] = 0;
		sceneRenderer->projectionMatrix[14] = -(camera.cameraDistanceMax+camera.cameraDistanceMin)/(camera.cameraDistanceMax-camera.cameraDistanceMin);
		sceneRenderer->projectionMatrix[15] = 1;
	}
	else if (camera.type == VuoSceneObjectSubType_FisheyeCamera)
	{
		bzero(sceneRenderer->projectionMatrix, sizeof(float)*16);
#if LIBRARY_PREMIUM_AVAILABLE
		VuoSceneRendererPremium_generateFisheyeProjectionMatrix(sceneRenderer, camera, aspectRatio);
#endif
	}
	else
		VUserLog("Unknown type %d", camera.type);


	// Transform the scene by the inverse of the camera's transform.
	// (Don't move the ship around the universe: move the universe around the ship.)
	{
		float cameraMatrix[16];
		VuoTransform_getMatrix(camera.transform, cameraMatrix);

		float invertedCameraMatrix[16];
		VuoTransform_invertMatrix4x4(cameraMatrix, invertedCameraMatrix);

		VuoTransform_copyMatrix4x4(invertedCameraMatrix, sceneRenderer->cameraMatrixInverse);
	}


	VuoSceneObject_retain(camera);
	VuoSceneObject_release(sceneRenderer->camera);
	sceneRenderer->camera = camera;
}

/**
 * Overwrites `address` with a null-terminated string consisting of
 * `i` (converted to string, assumed to be between 0 and 19 inclusive)
 * followed by `suffix`.
 */
static void VuoSceneRenderer_addUniformSuffix(char *address, int i, const char *suffix)
{
	if (i < 10)
		*(address++) = '0' + i;
	else
	{
		*(address++) = '1';
		*(address++) = '0' + (i - 10);
	}

	strcpy(address, suffix);
}

/**
 * Checks to ensure the currently-active VBO is correctly configured.
 *
 * This is time-consuming (since it downloads data from the GPU), so it should be used sparingly (e.g., in debug mode).
 */
void VuoSceneRenderer_checkDataBounds(CGLContextObj cgl_ctx, VuoSubmesh submesh, GLuint positionAttribute)
{
	GLint enabled = -1;
	glGetVertexAttribiv(positionAttribute, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
	if (enabled != 1)
	{
		VUserLog("Error: Vertex attrib array isn't enabled.");
		return;
	}

	GLint vaovbo = -1;
	glGetVertexAttribiv(positionAttribute, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &vaovbo);
	if (vaovbo != (GLint)submesh.glUpload.combinedBuffer)
		VUserLog("Error: GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING (%d) doesn't match submesh's VBO (%d).", vaovbo, submesh.glUpload.combinedBuffer);

	GLint vertexBufferBound = -1;
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vertexBufferBound);
	if (vertexBufferBound)
		VUserLog("Error: GL_ARRAY_BUFFER_BINDING is %d (it should probably be 0, since we're using a VAO).", vertexBufferBound);

	GLint elementBufferBound = -1;
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elementBufferBound);
	if (submesh.elementCount && !elementBufferBound)
		VUserLog("Error: GL_ELEMENT_ARRAY_BUFFER_BINDING is 0 (it should be nonzero, since we're trying to draw elements).");

	GLint combinedBufferSize = -1;
	glBindBuffer(GL_ARRAY_BUFFER, submesh.glUpload.combinedBuffer);
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &combinedBufferSize);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if (combinedBufferSize < 4)
		VUserLog("Error: combinedBuffer is unrealistically small (%d bytes).", combinedBufferSize);

	GLint attribStride = -1;
	glGetVertexAttribiv(positionAttribute, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &attribStride);

	GLint attribComponents = -1;
	glGetVertexAttribiv(positionAttribute, GL_VERTEX_ATTRIB_ARRAY_SIZE, &attribComponents);

	GLint attribType = -1;
	glGetVertexAttribiv(positionAttribute, GL_VERTEX_ATTRIB_ARRAY_TYPE, &attribType);

	void *attribBase = (void *)0;
	glGetVertexAttribPointerv(positionAttribute, GL_VERTEX_ATTRIB_ARRAY_POINTER, &attribBase);

	unsigned int bytesPerComponent;
	if (attribType == GL_UNSIGNED_INT)
		bytesPerComponent = 1;
	else if (attribType == GL_FLOAT)
		bytesPerComponent = 4;
	else
	{
		VUserLog("Error: Unknown combinedBuffer type %s.", VuoGl_stringForConstant((GLenum)attribType));
		return;
	}

	unsigned int minIndex = 0;
	unsigned int maxIndex = 0;
	if (submesh.elementCount)
	{
		if (submesh.glUpload.elementBufferSize != submesh.elementCount * sizeof(unsigned int))
			VUserLog("Error: elementBufferSize doesn't match elementCount.");

		unsigned int *b = (unsigned int *)malloc(submesh.glUpload.elementBufferSize);
		glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, submesh.glUpload.elementBufferSize, b);

		for (unsigned int i = 0; i < submesh.elementCount; ++i)
		{
			if (b[i] > maxIndex)
				maxIndex = b[i];
			if (b[i] < minIndex)
				minIndex = b[i];
		}

		free(b);
	}
	else
		maxIndex = submesh.vertexCount - 1;

	long startWithinVBO = (long)attribBase + (unsigned int)attribStride * minIndex;
	long   endWithinVBO = (long)attribBase +(unsigned int) attribStride * maxIndex + (unsigned int)attribComponents * bytesPerComponent;

	if (startWithinVBO < 0)
		VUserLog("Error: start < 0");
	if (endWithinVBO < 0)
		VUserLog("Error: end < 0");
	if (startWithinVBO > endWithinVBO)
		VUserLog("Error: start > end");
	if (endWithinVBO > combinedBufferSize)
		VUserLog("Error: end > combinedBufferSize");

	VGL();
}

/**
 * Draws @c so (using the uploaded object names in @c soi).
 * Does not traverse child objects.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
static void VuoSceneRenderer_drawSceneObject(VuoSceneObject so, VuoSceneRendererInternal_object *soi, float projectionMatrix[16], float modelviewMatrix[16], VuoSceneRendererInternal *sceneRenderer, VuoGlContext glContext)
{
	// Apply the overrides just within this function's scope.
	if (soi->overrideMesh)
		so.mesh = soi->overrideMesh;
	if (soi->overrideIsRealSize != VuoSceneRendererInternal_object::RealSize_Inherit)
		so.isRealSize = soi->overrideIsRealSize;
	if (soi->overrideShader)
		so.shader = soi->overrideShader;


	if (!so.mesh || !so.mesh->submeshCount || !so.shader)
		return;

	VuoImage image = VuoShader_getUniform_VuoImage(so.shader, "texture");
	if (so.isRealSize && !image)
		return;

	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

#ifdef PROFILE
	GLuint timeElapsedQuery;
	double timeStart = 0;
	SInt32 macMinorVersion;
	Gestalt(gestaltSystemVersionMinor, &macMinorVersion);
	if (macMinorVersion < 9)
	{
		// Prior to OS X v10.9, glGetQueryObjectuiv() isn't likely to work.
		// (On NVIDIA GeForce 9400M on OS X v10.8, it hangs for 6 seconds then returns bogus data.)
		// https://www.mail-archive.com/mac-opengl@lists.apple.com/msg00003.html
		// https://b33p.net/kosada/node/10677
		glFinish();
		timeStart = VuoLogGetTime();
	}
	else
	{
		glGenQueries(1, &timeElapsedQuery);
		glBeginQuery(GL_TIME_ELAPSED_EXT, timeElapsedQuery);
	}
#endif

	GLint positionAttribute;
	if (VuoIsDebugEnabled())
		VuoShader_getAttributeLocations(so.shader, so.mesh->submeshes[0].elementAssemblyMethod, cgl_ctx, &positionAttribute, NULL, NULL, NULL, NULL);


	// All VuoSubmeshes are assumed to have the same elementAssemblyMethod.
	VuoGlProgram program;
	if (!VuoShader_activate(so.shader, so.mesh->submeshes[0].elementAssemblyMethod, cgl_ctx, &program))
	{
		VUserLog("Shader activation failed.");
		return;
	}

	{
		GLint projectionMatrixUniform = VuoGlProgram_getUniformLocation(program, "projectionMatrix");
		glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, projectionMatrix);

		GLint useFisheyeProjectionUniform = VuoGlProgram_getUniformLocation(program, "useFisheyeProjection");
		if (useFisheyeProjectionUniform != -1)
			glUniform1i(useFisheyeProjectionUniform, sceneRenderer->camera.type == VuoSceneObjectSubType_FisheyeCamera);

		GLint modelviewMatrixUniform = VuoGlProgram_getUniformLocation(program, "modelviewMatrix");

		if (so.isRealSize)
		{
			float billboardMatrix[16];
			VuoTransform_getBillboardMatrix(image->pixelsWide, image->pixelsHigh, image->scaleFactor, so.preservePhysicalSize, modelviewMatrix[12], modelviewMatrix[13], sceneRenderer->viewportWidth, sceneRenderer->viewportHeight, sceneRenderer->backingScaleFactor, so.mesh->submeshes[0].positions[0].x, billboardMatrix);
			glUniformMatrix4fv(modelviewMatrixUniform, 1, GL_FALSE, billboardMatrix);
		}
		else
			glUniformMatrix4fv(modelviewMatrixUniform, 1, GL_FALSE, modelviewMatrix);

		GLint cameraMatrixInverseUniform = VuoGlProgram_getUniformLocation(program, "cameraMatrixInverse");
		if (cameraMatrixInverseUniform != -1)
			glUniformMatrix4fv(cameraMatrixInverseUniform, 1, GL_FALSE, sceneRenderer->cameraMatrixInverse);

		GLint cameraPositionUniform = VuoGlProgram_getUniformLocation(program, "cameraPosition");
		if (cameraPositionUniform != -1)
			glUniform3f(cameraPositionUniform, sceneRenderer->camera.transform.translation.x, sceneRenderer->camera.transform.translation.y, sceneRenderer->camera.transform.translation.z);

		GLint aspectRatioUniform = VuoGlProgram_getUniformLocation(program, "aspectRatio");
		if (aspectRatioUniform != -1)
			glUniform1f(aspectRatioUniform, (float)sceneRenderer->viewportWidth/(float)sceneRenderer->viewportHeight);

		GLint viewportSizeUniform = VuoGlProgram_getUniformLocation(program, "viewportSize");
		if (viewportSizeUniform != -1)
			glUniform2f(viewportSizeUniform, (float)sceneRenderer->viewportWidth, (float)sceneRenderer->viewportHeight);

		GLint ambientColorUniform = VuoGlProgram_getUniformLocation(program, "ambientColor");
		if (ambientColorUniform != -1)
			glUniform4f(ambientColorUniform,
						sceneRenderer->ambientColor.r * sceneRenderer->ambientColor.a,
						sceneRenderer->ambientColor.g * sceneRenderer->ambientColor.a,
						sceneRenderer->ambientColor.b * sceneRenderer->ambientColor.a,
						sceneRenderer->ambientColor.a);
		GLint ambientBrightnessUniform = VuoGlProgram_getUniformLocation(program, "ambientBrightness");
		if (ambientBrightnessUniform != -1)
			glUniform1f(ambientBrightnessUniform, sceneRenderer->ambientBrightness);

		char uniformName[27]; // strlen("pointLights[15].brightness")+1

		int pointLightCount = VuoListGetCount_VuoSceneObject(sceneRenderer->pointLights);
		GLint pointLightCountUniform = VuoGlProgram_getUniformLocation(program, "pointLightCount");
		if (pointLightCountUniform != -1)
		{
			glUniform1i(pointLightCountUniform, pointLightCount);

			strcpy(uniformName, "pointLights[");
			const int prefixLength = 12; // strlen("pointLights[")

			for (int i=1; i<=pointLightCount; ++i)
			{
				VuoSceneObject pointLight = VuoListGetValue_VuoSceneObject(sceneRenderer->pointLights, i);

				VuoSceneRenderer_addUniformSuffix(uniformName+prefixLength, i-1, "].color");
				GLint colorUniform = VuoGlProgram_getUniformLocation(program, uniformName);
				glUniform4f(colorUniform,
							pointLight.lightColor.r * pointLight.lightColor.a,
							pointLight.lightColor.g * pointLight.lightColor.a,
							pointLight.lightColor.b * pointLight.lightColor.a,
							pointLight.lightColor.a);

				VuoSceneRenderer_addUniformSuffix(uniformName+prefixLength, i-1, "].brightness");
				GLint brightnessUniform = VuoGlProgram_getUniformLocation(program, uniformName);
				glUniform1f(brightnessUniform, pointLight.lightBrightness);

				VuoSceneRenderer_addUniformSuffix(uniformName+prefixLength, i-1, "].position");
				GLint positionUniform = VuoGlProgram_getUniformLocation(program, uniformName);
				glUniform3f(positionUniform, pointLight.transform.translation.x, pointLight.transform.translation.y, pointLight.transform.translation.z);

				VuoSceneRenderer_addUniformSuffix(uniformName+prefixLength, i-1, "].range");
				GLint rangeUniform = VuoGlProgram_getUniformLocation(program, uniformName);
				glUniform1f(rangeUniform, pointLight.lightRange);

				VuoSceneRenderer_addUniformSuffix(uniformName+prefixLength, i-1, "].sharpness");
				GLint sharpnessUniform = VuoGlProgram_getUniformLocation(program, uniformName);
				glUniform1f(sharpnessUniform, pointLight.lightSharpness);
			}
		}

		int spotLightCount = VuoListGetCount_VuoSceneObject(sceneRenderer->spotLights);
		GLint spotLightCountUniform = VuoGlProgram_getUniformLocation(program, "spotLightCount");
		if (spotLightCountUniform != -1)
		{
			glUniform1i(spotLightCountUniform, spotLightCount);

			strcpy(uniformName, "spotLights[");
			const int prefixLength = 11; // strlen("spotLights[")

			for (int i=1; i<=spotLightCount; ++i)
			{
				VuoSceneObject spotLight = VuoListGetValue_VuoSceneObject(sceneRenderer->spotLights, i);

				VuoSceneRenderer_addUniformSuffix(uniformName+prefixLength, i-1, "].color");
				GLint colorUniform = VuoGlProgram_getUniformLocation(program, uniformName);
				glUniform4f(colorUniform,
							spotLight.lightColor.r * spotLight.lightColor.a,
							spotLight.lightColor.g * spotLight.lightColor.a,
							spotLight.lightColor.b * spotLight.lightColor.a,
							spotLight.lightColor.a);

				VuoSceneRenderer_addUniformSuffix(uniformName+prefixLength, i-1, "].brightness");
				GLint brightnessUniform = VuoGlProgram_getUniformLocation(program, uniformName);
				glUniform1f(brightnessUniform, spotLight.lightBrightness);

				VuoSceneRenderer_addUniformSuffix(uniformName+prefixLength, i-1, "].position");
				GLint positionUniform = VuoGlProgram_getUniformLocation(program, uniformName);
				glUniform3f(positionUniform, spotLight.transform.translation.x, spotLight.transform.translation.y, spotLight.transform.translation.z);

				VuoSceneRenderer_addUniformSuffix(uniformName+prefixLength, i-1, "].direction");
				GLint directionUniform = VuoGlProgram_getUniformLocation(program, uniformName);
				VuoPoint3d direction = VuoTransform_getDirection(spotLight.transform);
				glUniform3f(directionUniform, direction.x, direction.y, direction.z);

				VuoSceneRenderer_addUniformSuffix(uniformName+prefixLength, i-1, "].cone");
				GLint coneUniform = VuoGlProgram_getUniformLocation(program, uniformName);
				glUniform1f(coneUniform, spotLight.lightCone);

				VuoSceneRenderer_addUniformSuffix(uniformName+prefixLength, i-1, "].range");
				GLint rangeUniform = VuoGlProgram_getUniformLocation(program, uniformName);
				glUniform1f(rangeUniform, spotLight.lightRange);

				VuoSceneRenderer_addUniformSuffix(uniformName+prefixLength, i-1, "].sharpness");
				GLint sharpnessUniform = VuoGlProgram_getUniformLocation(program, uniformName);
				glUniform1f(sharpnessUniform, spotLight.lightSharpness);
			}
		}

		VuoSceneRenderer_setGLDepthMask(!so.shader->isTransparent);

		VuoSceneRenderer_setGL(GL_SAMPLE_ALPHA_TO_COVERAGE, so.shader->useAlphaAsCoverage);
		VuoSceneRenderer_setGL(GL_SAMPLE_ALPHA_TO_ONE,      so.shader->useAlphaAsCoverage);

		if (so.blendMode == VuoBlendMode_Normal)
		{
			VuoSceneRenderer_setGL(GL_DEPTH_TEST, true);
			VuoSceneRenderer_setGLBlendFunction(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			VuoSceneRenderer_setGLBlendEquation(GL_FUNC_ADD, GL_FUNC_ADD);
		}
		else
		{
			VuoSceneRenderer_setGL(GL_DEPTH_TEST, false);

			if (so.blendMode == VuoBlendMode_Multiply)
			{
				VuoSceneRenderer_setGLBlendFunction(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				VuoSceneRenderer_setGLBlendEquation(GL_FUNC_ADD, GL_FUNC_ADD);
			}
			else if (so.blendMode == VuoBlendMode_DarkerComponents)
			{
				/// @todo https://b33p.net/kosada/node/12014
				VuoSceneRenderer_setGLBlendFunction(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
				VuoSceneRenderer_setGLBlendEquation(GL_MIN, GL_FUNC_ADD);
			}
			else if (so.blendMode == VuoBlendMode_LighterComponents)
			{
				VuoSceneRenderer_setGLBlendFunction(GL_ZERO, GL_ZERO, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				VuoSceneRenderer_setGLBlendEquation(GL_MAX, GL_FUNC_ADD);
			}
			else if (so.blendMode == VuoBlendMode_LinearDodge)
			{
				VuoSceneRenderer_setGLBlendFunction(GL_ONE, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				VuoSceneRenderer_setGLBlendEquation(GL_FUNC_ADD, GL_FUNC_ADD);
			}
			else if (so.blendMode == VuoBlendMode_Subtract)
			{
				VuoSceneRenderer_setGLBlendFunction(GL_ONE, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				VuoSceneRenderer_setGLBlendEquation(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD);
			}
		}

			glBindVertexArray(soi->vao);

			VuoSubmesh submesh = so.mesh->submeshes[0];
			GLenum mode = VuoSubmesh_getGlMode(submesh);

			GLint hasTextureCoordinatesUniform = VuoGlProgram_getUniformLocation(program, "hasTextureCoordinates");
			if (hasTextureCoordinatesUniform != -1)
				glUniform1i(hasTextureCoordinatesUniform, submesh.glUpload.textureCoordinateOffset != NULL);

			GLint primitiveHalfSizeUniform = VuoGlProgram_getUniformLocation(program, "primitiveHalfSize");
			if (primitiveHalfSizeUniform != -1)
				glUniform1f(primitiveHalfSizeUniform, submesh.primitiveSize/2);

			bool enableCulling = submesh.faceCullingMode != GL_NONE && !so.shader->isTransparent;
			VuoSceneRenderer_setGL(GL_CULL_FACE, enableCulling);
			if (enableCulling)
				VuoSceneRenderer_setGLFaceCulling(submesh.faceCullingMode);

			if (VuoIsDebugEnabled())
			{
				VGL();
				VuoSceneRenderer_checkDataBounds(cgl_ctx, submesh, positionAttribute);
			}

			unsigned long completeInputElementCount = VuoSubmesh_getCompleteElementCount(submesh);
			if (submesh.elementCount)
				glDrawElements(mode, completeInputElementCount, GL_UNSIGNED_INT, (void*)0);
			else if (submesh.vertexCount)
				glDrawArrays(mode, 0, completeInputElementCount);

			glBindVertexArray(0);
	}
	VuoShader_deactivate(so.shader, so.mesh->submeshes[0].elementAssemblyMethod, cgl_ctx);

#ifdef PROFILE
	double seconds;
	if (macMinorVersion < 9)
	{
		glFinish();
		seconds = VuoLogGetTime() - timeStart;
	}
	else
	{
		glEndQuery(GL_TIME_ELAPSED_EXT);
		GLuint nanoseconds;
		glGetQueryObjectuiv(timeElapsedQuery, GL_QUERY_RESULT, &nanoseconds);
		seconds = ((double)nanoseconds) / NSEC_PER_SEC;
		glDeleteQueries(1, &timeElapsedQuery);
	}

	std::string description = so.name ? std::string(so.name) : "no name";
	description += std::string(" (") + so.shader->name + ")";
	sceneRenderer->profileTimes.push_back(make_pair(description, seconds));
#endif
}

//static void VuoSceneRenderer_drawElement(VuoSceneObject so, float projectionMatrix[16], float compositeModelviewMatrix[16], VuoGlContext glContext, int element, float length);

/**
 * State for rendering an object at its particular place in the hierarchy.
 */
typedef struct
{
	VuoSceneObject *so;
	VuoSceneRendererInternal_object *soi;
	float modelviewMatrix[16];
} VuoSceneRenderer_treeRenderState;

/**
 * Draws @c so and its child objects.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
static void VuoSceneRenderer_drawSceneObjectsRecursively(VuoSceneRendererInternal *sceneRenderer, VuoGlContext glContext)
{
	VuoSceneRenderer_treeRenderState rootState;
	rootState.so = &sceneRenderer->rootSceneObject;
	rootState.soi = &sceneRenderer->rootSceneObjectInternal;
	VuoTransform_getMatrix(VuoTransform_makeIdentity(), rootState.modelviewMatrix);

	std::list<VuoSceneRenderer_treeRenderState> objectsToRender(1, rootState);
	while (!objectsToRender.empty())
	{
		VuoSceneRenderer_treeRenderState currentState = objectsToRender.front();
		objectsToRender.pop_front();

		float localModelviewMatrix[16];
		VuoTransform_getMatrix(currentState.so->transform, localModelviewMatrix);
		float compositeModelviewMatrix[16];
		VuoTransform_multiplyMatrices4x4(localModelviewMatrix, currentState.modelviewMatrix, compositeModelviewMatrix);

		VuoSceneRenderer_drawSceneObject(*currentState.so, currentState.soi, sceneRenderer->projectionMatrix, compositeModelviewMatrix, sceneRenderer, glContext);
//		VuoSceneRenderer_drawElement(currentState.so, sceneRenderer->projectionMatrix, currentState.modelviewMatrix, sceneRenderer->glContext, 0, .08f);	// Normals
//		VuoSceneRenderer_drawElement(currentState.so, sceneRenderer->projectionMatrix, currentState.modelviewMatrix, sceneRenderer->glContext, 1, .08f);	// Tangents
//		VuoSceneRenderer_drawElement(currentState.so, sceneRenderer->projectionMatrix, currentState.modelviewMatrix, sceneRenderer->glContext, 2, .08f);	// Bitangents

		// Prepend childObjects to the objectsToRender queue.
		// (Do it in reverse order so the objects end up in forward order after calling ::push_front repeatedly.)
		VuoSceneObject *childObjects = VuoListGetData_VuoSceneObject(currentState.so->childObjects);
		for (long i = VuoListGetCount_VuoSceneObject(currentState.so->childObjects) - 1; i >= 0; --i)
		{
			VuoSceneRenderer_treeRenderState childState;
			childState.so = &(childObjects[i]);
			childState.soi = &(currentState.soi->childObjects[i]);
			memcpy(childState.modelviewMatrix, compositeModelviewMatrix, sizeof(float[16]));
			objectsToRender.push_front(childState);
		}
	}
}

//static void VuoSceneRenderer_drawLights(VuoSceneRendererInternal *sceneRenderer);
static void VuoSceneRenderer_cleanupSceneObjectsRecursively(VuoSceneObject so, VuoSceneRendererInternal_object *soi, VuoGlContext glContext);
static void VuoSceneRenderer_cleanupMeshShaderItems(VuoSceneRendererInternal *sceneRenderer, VuoGlContext glContext);
static void VuoSceneRenderer_uploadSceneObjectsRecursively(VuoSceneRendererInternal *sceneRenderer, VuoSceneObject so, VuoSceneRendererInternal_object *soi, VuoGlContext glContext);

/**
 * Uploads the scene to the GPU (if it's changed), and renders it.
 *
 * @threadAnyGL
 */
extern "C" void VuoSceneRenderer_draw(VuoSceneRenderer sr)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;

	if (!sceneRenderer->rootSceneObjectPendingValid && !sceneRenderer->rootSceneObjectValid)
		return;

	VuoGlContext_perform(^(CGLContextObj cgl_ctx){

	if (sceneRenderer->rootSceneObjectPendingUpdated)
	{
		// Release the old scenegraph.
		if (sceneRenderer->rootSceneObjectValid)
		{
			VuoSceneRenderer_cleanupSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, cgl_ctx);
			VuoSceneRenderer_cleanupMeshShaderItems(sceneRenderer, cgl_ctx);
			VuoSceneObject_release(sceneRenderer->rootSceneObject);
			VuoRelease(sceneRenderer->pointLights);
			VuoRelease(sceneRenderer->spotLights);
		}

		// Upload the new scenegraph.
		sceneRenderer->rootSceneObject = sceneRenderer->rootSceneObjectPending;
		VuoSceneObject_retain(sceneRenderer->rootSceneObject);
		VuoSceneRenderer_uploadSceneObjectsRecursively(sceneRenderer, sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, cgl_ctx);

		VuoSceneObject_findLights(sceneRenderer->rootSceneObject, &sceneRenderer->ambientColor, &sceneRenderer->ambientBrightness, &sceneRenderer->pointLights, &sceneRenderer->spotLights);
		VuoRetain(sceneRenderer->pointLights);
		VuoRetain(sceneRenderer->spotLights);

		sceneRenderer->rootSceneObjectValid = true;
		sceneRenderer->needToRegenerateProjectionMatrix = true;
		sceneRenderer->rootSceneObjectPendingUpdated = false;
	}

	if (sceneRenderer->needToRegenerateProjectionMatrix)
	{
		VuoSceneRenderer_regenerateProjectionMatrixInternal(sceneRenderer);
		sceneRenderer->needToRegenerateProjectionMatrix = false;
	}


	// Start out with VuoGlContextPool::createContext()'s initial state.
	bzero(&sceneRenderer->glState, sizeof(sceneRenderer->glState));
	sceneRenderer->glState.vGL_DEPTH_MASK               = true;
	sceneRenderer->glState.vGL_DEPTH_TEST               = false;
	sceneRenderer->glState.vGL_CULL_FACE                = true;
	sceneRenderer->glState.vGL_CULL_FACE_MODE           = GL_BACK;
	sceneRenderer->glState.vGL_BLEND_SRC_RGB            = GL_ONE;
	sceneRenderer->glState.vGL_BLEND_DST_RGB            = GL_ONE_MINUS_SRC_ALPHA;
	sceneRenderer->glState.vGL_BLEND_SRC_ALPHA          = GL_ONE;
	sceneRenderer->glState.vGL_BLEND_DST_ALPHA          = GL_ONE_MINUS_SRC_ALPHA;
	sceneRenderer->glState.vGL_BLEND_EQUATION_RGB       = GL_FUNC_ADD;
	sceneRenderer->glState.vGL_BLEND_EQUATION_ALPHA     = GL_FUNC_ADD;
	sceneRenderer->glState.vGL_SAMPLE_ALPHA_TO_COVERAGE = false;
	sceneRenderer->glState.vGL_SAMPLE_ALPHA_TO_ONE      = false;

	VuoSceneRenderer_drawSceneObjectsRecursively(sceneRenderer, cgl_ctx);

	// Restore the context back to VuoGlContextPool::createContext()'s initial state.
	VuoSceneRenderer_setGLDepthMask(                    true );
	VuoSceneRenderer_setGL(GL_DEPTH_TEST,               false);
	VuoSceneRenderer_setGL(GL_CULL_FACE,                true );
	VuoSceneRenderer_setGLFaceCulling(GL_BACK);
	VuoSceneRenderer_setGLBlendFunction(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	VuoSceneRenderer_setGLBlendEquation(GL_FUNC_ADD, GL_FUNC_ADD);
	VuoSceneRenderer_setGL(GL_SAMPLE_ALPHA_TO_COVERAGE, false);
	VuoSceneRenderer_setGL(GL_SAMPLE_ALPHA_TO_ONE,      false);


//	VuoSceneRenderer_drawLights(sceneRenderer);


#if LIBRARY_PREMIUM_AVAILABLE
	if (sceneRenderer->camera.type == VuoSceneObjectSubType_FisheyeCamera)
		VuoSceneRendererPremium_drawVignette(sceneRenderer, cgl_ctx);
#endif

#ifdef PROFILE
	VL();
	VLog("Object render time (percent of a 60 Hz frame)");
	double totalPercent = 0;
	sceneRenderer->profileTimes.sort(VuoProfileSort);
	for (std::list<VuoProfileEntry>::iterator i = sceneRenderer->profileTimes.begin(); i != sceneRenderer->profileTimes.end(); ++i)
	{
		double objectPercent = i->second / (1./60.) * 100.;
		VLog("	%6.2f  %s", objectPercent, i->first.c_str());
		totalPercent += objectPercent;
	}
	VLog("	------  -----");
	VLog("	%6.2f  total", totalPercent);
	sceneRenderer->profileTimes.clear();
#endif

	glFlushRenderAPPLE();

	if (VuoIsDebugEnabled())
	{
		GLint rendererID;
		CGLGetParameter(cgl_ctx, kCGLCPCurrentRendererID, &rendererID);
		if (rendererID != sceneRenderer->glContextRendererID)
		{
			VUserLog("OpenGL context %p's renderer changed to %s", cgl_ctx, VuoCglRenderer_getText(rendererID));
			sceneRenderer->glContextRendererID = rendererID;
		}
	}

	});
}

/**
 * Draws all vertex normals in @c so.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
/*static void VuoSceneRenderer_drawElement(VuoSceneObject so, float projectionMatrix[16], float compositeModelviewMatrix[16], VuoGlContext glContext, int element, float length)	// TODO Enum type for element to debug
{
	if (!so.mesh)
		return;

	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projectionMatrix);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(compositeModelviewMatrix);

	glBegin(GL_LINES);
	bool first=true;
	for (unsigned int i = 0; i < so.mesh->submeshCount; i++)
	{
		VuoSubmesh submesh = so.mesh->submeshes[i];
		VuoSubmesh_download(&submesh);

		for(unsigned int m = 0; m < submesh.vertexCount; m++)
		{
			VuoPoint4d v = submesh.positions[m];
			VuoPoint4d n = (VuoPoint4d){0,0,1,1};

			switch(element)
			{
				case 0:	// normals
					n = submesh.normals[m];
					glColor3f(1,.5,.5);
					break;
				case 1:	// tangents
					n = submesh.tangents[m];
					glColor3f(.5,1,.5);
					break;
				case 2:	// bitangents
					n = submesh.bitangents[m];
					glColor3f(.5,.5,1);
					break;
			}

			if (first)
			{
				VuoPoint3d n3 = VuoPoint3d_make(n.x,n.y,n.z);
				VUserLog("%p [%d]: %s, length %g",&so,element,VuoPoint4d_getSummary(n),VuoPoint3d_magnitude(n3));

				// Check orthogonality
				{
					VuoPoint3d n3 = VuoPoint3d_make(submesh.normals[m].x,submesh.normals[m].y,submesh.normals[m].z);
					VuoPoint3d t3 = VuoPoint3d_make(submesh.tangents[m].x,submesh.tangents[m].y,submesh.tangents[m].z);
					VuoPoint3d b3 = VuoPoint3d_make(submesh.bitangents[m].x,submesh.bitangents[m].y,submesh.bitangents[m].z);

					if (VuoPoint3d_dotProduct(n3,t3) > 0.0001)
						VUserLog("	n•t = %g; should be 0",VuoPoint3d_dotProduct(n3,t3));
					if (VuoPoint3d_dotProduct(n3,b3) > 0.0001)
						VUserLog("	n•b = %g; should be 0",VuoPoint3d_dotProduct(n3,b3));
				}
			}
			first=false;

			n = VuoPoint4d_add(v, VuoPoint4d_multiply(n, length));

			glVertex3d(v.x, v.y, v.z);
			glVertex3d(n.x, n.y, n.z);
		}
	}
	glEnd();

	glLoadIdentity();
}*/

/**
 * Draws a circle using OpenGL immediate mode.  For debugging only.
 */
/*static void drawCircle(CGLContextObj cgl_ctx, VuoPoint3d center, float radius, VuoPoint3d normal)
{
	glPushMatrix();
	VuoTransform t = VuoTransform_makeQuaternion(
				center,
				VuoTransform_quaternionFromVectors(VuoPoint3d_make(1,0,0), normal),
				VuoPoint3d_make(1,1,1));
	float m[16];
	VuoTransform_getMatrix(t, m);
	glLoadMatrixf(m);

	glBegin(GL_LINE_LOOP);
	const int segments = 32;
	for (int i=0; i<segments; ++i)
		glVertex3f(
					0,
					cos(2.*M_PI*i/segments)*radius,
					sin(2.*M_PI*i/segments)*radius);
	glEnd();

	glPopMatrix();
}*/

/**
 * Draws a cone using OpenGL immediate mode.  For debugging only.
 */
/*static void drawCone(CGLContextObj cgl_ctx, VuoPoint3d center, float radius, VuoPoint3d normal, float height)
{
	glPushMatrix();
	VuoTransform t = VuoTransform_makeQuaternion(
				center,
				VuoTransform_quaternionFromVectors(VuoPoint3d_make(1,0,0), normal),
				VuoPoint3d_make(1,1,1));
	float m[16];
	VuoTransform_getMatrix(t, m);
	glLoadMatrixf(m);

	glBegin(GL_LINE_LOOP);
	const int segments = 32;
	for (int i=0; i<segments; ++i)
		glVertex3f(
					0,
					cos(2.*M_PI*i/segments)*radius,
					sin(2.*M_PI*i/segments)*radius);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(-height, 0, 0);	glVertex3f(0,  radius,       0);
		glVertex3f(-height, 0, 0);	glVertex3f(0,       0,  radius);
		glVertex3f(-height, 0, 0);	glVertex3f(0, -radius,       0);
		glVertex3f(-height, 0, 0);	glVertex3f(0,       0, -radius);
	glEnd();

	glPopMatrix();
}*/

/**
 * Draws the scene's point and spot lights.
 */
/*static void VuoSceneRenderer_drawLights(VuoSceneRendererInternal *sceneRenderer)
{
	CGLContextObj cgl_ctx = (CGLContextObj)sceneRenderer->glContext;

	glPointSize(20);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(sceneRenderer->projectionMatrix);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	int pointLightCount = VuoListGetCount_VuoSceneObject(sceneRenderer->pointLights);
	for (int i=1; i<=pointLightCount; ++i)
	{
		VuoSceneObject pointLight = VuoListGetValue_VuoSceneObject(sceneRenderer->pointLights, i);
		VuoPoint3d position = pointLight.transform.translation;
		glBegin(GL_POINTS);
			glVertex3f(position.x, position.y, position.z);
		glEnd();

		// Draw a pair of concentric sphere outlines illustrating the light's range.
		{
			float innerRange = pointLight.lightRange*pointLight.lightSharpness;
			float outerRange = pointLight.lightRange*(2-pointLight.lightSharpness);

			// XY plane
			drawCircle(cgl_ctx, position, innerRange, VuoPoint3d_make(0,0,1));
			drawCircle(cgl_ctx, position, outerRange, VuoPoint3d_make(0,0,1));
			// XZ plane
			drawCircle(cgl_ctx, position, innerRange, VuoPoint3d_make(0,1,0));
			drawCircle(cgl_ctx, position, outerRange, VuoPoint3d_make(0,1,0));
			// YZ plane
			drawCircle(cgl_ctx, position, innerRange, VuoPoint3d_make(1,0,0));
			drawCircle(cgl_ctx, position, outerRange, VuoPoint3d_make(1,0,0));
		}
	}

	int spotLightCount = VuoListGetCount_VuoSceneObject(sceneRenderer->spotLights);
	for (int i=1; i<=spotLightCount; ++i)
	{
		VuoSceneObject spotLight = VuoListGetValue_VuoSceneObject(sceneRenderer->spotLights, i);
		VuoPoint3d position = spotLight.transform.translation;
		glBegin(GL_POINTS);
			glVertex3f(position.x, position.y, position.z);
		glEnd();

		VuoPoint3d direction = VuoTransform_getDirection(spotLight.transform);
		float innerRange = spotLight.lightRange*spotLight.lightSharpness;
		float outerRange = spotLight.lightRange*(2-spotLight.lightSharpness);

		// Draw a pair of cones (whose bases are the intersection of a plane and a spherical shell) to illustrate the light's range.
		{
			float innerConeAngle = spotLight.lightCone*spotLight.lightSharpness;
			float outerConeAngle = spotLight.lightCone*(2-spotLight.lightSharpness);

			float innerIntersectionDistance = innerRange*cos(innerConeAngle/2.);
			float outerIntersectionDistance = outerRange*cos(outerConeAngle/2.);

			float innerIntersectionRadius = innerRange*sin(innerConeAngle/2.);
			float outerIntersectionRadius = outerRange*sin(outerConeAngle/2.);

			VuoPoint3d innerEndpoint = VuoPoint3d_add(position, VuoPoint3d_multiply(direction, innerIntersectionDistance));
			VuoPoint3d outerEndpoint = VuoPoint3d_add(position, VuoPoint3d_multiply(direction, outerIntersectionDistance));

			drawCone(cgl_ctx, innerEndpoint, innerIntersectionRadius, direction, innerIntersectionDistance);
			drawCone(cgl_ctx, outerEndpoint, outerIntersectionRadius, direction, outerIntersectionDistance);
		}
	}
}*/

/**
 * Creates a mesh and image for the specified sceneobject.
 */
static void VuoSceneRenderer_renderText(VuoSceneRendererInternal *sceneRenderer, VuoSceneObject so, VuoSceneRendererInternal_object *soi)
{
	// Instead of modifying the VuoSceneObject itself
	// (which breaks Vuo's "data passed through cables is immutable" rule,
	// since rootSceneObject is output as renderedLayers),
	// use the shadow object to store overrides of the actual object.

	// soi->overrideMesh = VuoMesh_makeQuadWithoutNormals();
	// VuoRetain(soi->overrideMesh);

	soi->overrideIsRealSize = VuoSceneRendererInternal_object::RealSize_True;

	VuoImage textImage = VuoImage_makeText(so.text, so.font, sceneRenderer->backingScaleFactor);
	soi->overrideShader = VuoShader_makeUnlitImageShader(textImage, 1);
	VuoRetain(soi->overrideShader);
}

/**
 * Binds the relevant (given the current shader) parts of @c so to an OpenGL Vertex Array Object.
 * Does not traverse child objects.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * If `cache` is true, the VAO is cached in sceneRenderer->meshShaderItems (which gets cleaned up by @ref VuoSceneRenderer_cleanupMeshShaderItems each time the root sceneobject is changed).
 *
 * @threadAnyGL
 */
static void VuoSceneRenderer_uploadSceneObject(VuoSceneRendererInternal *sceneRenderer, VuoSceneObject so, VuoSceneRendererInternal_object *soi, VuoGlContext glContext, bool cache)
{
	soi->overrideMesh = NULL;
	soi->overrideIsRealSize = VuoSceneRendererInternal_object::RealSize_Inherit;
	soi->overrideShader = NULL;

	if (so.type == VuoSceneObjectSubType_Text && so.text && so.text[0] != 0)
		VuoSceneRenderer_renderText(sceneRenderer, so, soi);

	// Apply the overrides just within this function's scope.
	if (soi->overrideMesh)
		so.mesh = soi->overrideMesh;
	if (soi->overrideShader)
		so.shader = soi->overrideShader;


	if (!so.mesh || !so.mesh->submeshCount || !so.shader)
		return;


	// If we already have a VAO for this mesh/shader combination, use it.
	VuoSceneRendererMeshShader meshShader(so.mesh, so.shader);
	VuoSceneRendererMeshShaderVAOs::iterator i = sceneRenderer->meshShaderItems.find(meshShader);
	if (cache && i != sceneRenderer->meshShaderItems.end())
	{
		soi->vao = i->second;
		return;
	}


	// …otherwise, create one.

	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	// Fetch the shader's attribute locations.
	// All VuoSubmeshes are assumed to have the same elementAssemblyMethod.
	GLint positionAttribute, normalAttribute, tangentAttribute, bitangentAttribute, textureCoordinateAttribute;
	VuoShader_getAttributeLocations(so.shader, so.mesh->submeshes[0].elementAssemblyMethod, glContext, &positionAttribute, &normalAttribute, &tangentAttribute, &bitangentAttribute, &textureCoordinateAttribute);


		VuoSubmesh meshItem = so.mesh->submeshes[0];
		GLuint vertexArray;


		// Create a Vertex Array Object, to store this sceneobject's vertex array bindings.
		glGenVertexArrays(1, &vertexArray);
		glBindVertexArray(vertexArray);


		// Bind the combined buffer to the Vertex Array Object.
		glBindBuffer(GL_ARRAY_BUFFER, so.mesh->submeshes[0].glUpload.combinedBuffer);


		// Populate the Vertex Array Object with the various vertex attributes.

		int stride = VuoSubmesh_getStride(so.mesh->submeshes[0]);
		glEnableVertexAttribArray((GLuint)positionAttribute);
		glVertexAttribPointer((GLuint)positionAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, stride, (void*)0);

		if (meshItem.glUpload.normalOffset && normalAttribute>=0)
		{
			glEnableVertexAttribArray((GLuint)normalAttribute);
			glVertexAttribPointer((GLuint)normalAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, stride, so.mesh->submeshes[0].glUpload.normalOffset);
		}

		if (meshItem.glUpload.tangentOffset && tangentAttribute>=0)
		{
			glEnableVertexAttribArray((GLuint)tangentAttribute);
			glVertexAttribPointer((GLuint)tangentAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, stride, so.mesh->submeshes[0].glUpload.tangentOffset);
		}

		if (meshItem.glUpload.bitangentOffset && bitangentAttribute>=0)
		{
			glEnableVertexAttribArray((GLuint)bitangentAttribute);
			glVertexAttribPointer((GLuint)bitangentAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, stride, so.mesh->submeshes[0].glUpload.bitangentOffset);
		}

		if (meshItem.glUpload.textureCoordinateOffset && textureCoordinateAttribute>=0)
		{
			glEnableVertexAttribArray((GLuint)textureCoordinateAttribute);
			glVertexAttribPointer((GLuint)textureCoordinateAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, stride, so.mesh->submeshes[0].glUpload.textureCoordinateOffset);
		}


		// Bind the Element Buffer to the Vertex Array Object
		if (so.mesh->submeshes[0].elementCount)
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, so.mesh->submeshes[0].glUpload.elementBuffer);


		glBindVertexArray(0);

		soi->vao = vertexArray;

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (cache)
		sceneRenderer->meshShaderItems[meshShader] = soi->vao;
}

/**
 * Uploads @c so and its child objects to the GPU, and stores the uploaded object names in @c soi.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
static void VuoSceneRenderer_uploadSceneObjectsRecursively(VuoSceneRendererInternal *sceneRenderer, VuoSceneObject so, VuoSceneRendererInternal_object *soi, VuoGlContext glContext)
{
	VuoSceneRenderer_uploadSceneObject(sceneRenderer, so, soi, glContext, true);

	if (!so.childObjects)
	{
		soi->childObjectCount = 0;
		soi->childObjects = NULL;
		return;
	}

	unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
	VuoSceneObject *childObjects = VuoListGetData_VuoSceneObject(so.childObjects);
	soi->childObjects = new VuoSceneRendererInternal_object[childObjectCount];
	soi->childObjectCount = childObjectCount;
	for (unsigned long i = 0; i < childObjectCount; ++i)
	{
		VuoSceneRendererInternal_object childObjectInternal;
		VuoSceneRenderer_uploadSceneObjectsRecursively(sceneRenderer, childObjects[i], &childObjectInternal, glContext);
		soi->childObjects[i] = childObjectInternal;
	}
}

/**
 * Cleans up internal state created by @c VuoSceneRenderer_uploadSceneObjectsRecursively.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
static void VuoSceneRenderer_cleanupSceneObjectsRecursively(VuoSceneObject so, VuoSceneRendererInternal_object *soi, VuoGlContext glContext)
{
	VuoSceneObject *childObjects = VuoListGetData_VuoSceneObject(so.childObjects);
	for (unsigned int i = 0; i < soi->childObjectCount; ++i)
		VuoSceneRenderer_cleanupSceneObjectsRecursively(childObjects[i], &(soi->childObjects[i]), glContext);

	if (soi->overrideMesh)
		VuoRelease(soi->overrideMesh);
	if (soi->overrideShader)
		VuoRelease(soi->overrideShader);

	if (soi->childObjects)
		delete[] soi->childObjects;
}

/**
 * Releases the GPU objects created by @c VuoSceneRenderer_uploadSceneObjectsRecursively.
 */
static void VuoSceneRenderer_cleanupMeshShaderItems(VuoSceneRendererInternal *sceneRenderer, VuoGlContext glContext)
{
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	for (VuoSceneRendererMeshShaderVAOs::iterator msi = sceneRenderer->meshShaderItems.begin(); msi != sceneRenderer->meshShaderItems.end(); ++msi)
			glDeleteVertexArrays(1, &(msi->second));

	sceneRenderer->meshShaderItems.clear();
}

/**
 * Changes the scenegraph to be rendered.
 *
 * This function retains the scene, but it isn't uploaded to the GPU until you call @ref VuoSceneRenderer_draw.
 *
 * @threadAny
 */
void VuoSceneRenderer_setRootSceneObject(VuoSceneRenderer sr, VuoSceneObject rootSceneObject)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;

	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	{
		VuoSceneObject_retain(rootSceneObject);

		if (sceneRenderer->rootSceneObjectPendingValid)
			VuoSceneObject_release(sceneRenderer->rootSceneObjectPending);

		sceneRenderer->rootSceneObjectPending = rootSceneObject;

		sceneRenderer->rootSceneObjectPendingValid = true;
		sceneRenderer->rootSceneObjectPendingUpdated = true;
	}
	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);
}

/**
 * Returns the scenegraph to be rendered.
 *
 * This function only returns valid data after @ref VuoSceneRenderer_draw has been called.
 *
 * @threadAny
 */
VuoSceneObject VuoSceneRenderer_getRootSceneObject(VuoSceneRenderer sr, bool *isValid)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;
	*isValid = sceneRenderer->rootSceneObjectValid;
	return sceneRenderer->rootSceneObject;
}

/**
 * Changes the name of the camera to look for.
 * The first camera whose name contains @c cameraName will be rendered (next time @c VuoSceneRenderer_draw() is called),
 * or, if no camera matches, @c VuoSceneObject_makeDefaultCamera() will be used.
 *
 * If `cameraName` is stereoscopic, `useLeftCamera` selects between the left and right cameras in the stereo pair.
 *
 * @threadAny
 */
void VuoSceneRenderer_setCameraName(VuoSceneRenderer sr, VuoText cameraName, VuoBoolean useLeftCamera)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;

	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	{
		if (sceneRenderer->cameraName)
			VuoRelease(sceneRenderer->cameraName);

		sceneRenderer->cameraName = cameraName;
		VuoRetain(sceneRenderer->cameraName);

		sceneRenderer->useLeftCamera = useLeftCamera;

		sceneRenderer->needToRegenerateProjectionMatrix = true;
	}
	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);
}

/**
 * Destroys and deallocates the scene renderer.
 *
 * @threadAny
 */
void VuoSceneRenderer_destroy(VuoSceneRenderer sr)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;

	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);

	if (sceneRenderer->rootSceneObjectPendingValid)
		VuoSceneObject_release(sceneRenderer->rootSceneObjectPending);

	if (sceneRenderer->rootSceneObjectValid)
	{
		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			VuoSceneRenderer_cleanupSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, cgl_ctx);
			VuoSceneRenderer_cleanupMeshShaderItems(sceneRenderer, cgl_ctx);
		});
		VuoSceneObject_release(sceneRenderer->rootSceneObject);

		VuoRelease(sceneRenderer->pointLights);
		VuoRelease(sceneRenderer->spotLights);
	}

	if (sceneRenderer->cameraName)
		VuoRelease(sceneRenderer->cameraName);
	VuoSceneObject_release(sceneRenderer->camera);

#if LIBRARY_PREMIUM_AVAILABLE
	VuoRelease(sceneRenderer->vignetteShader);
#endif

	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);
	dispatch_release(sceneRenderer->scenegraphSemaphore);

	free(sceneRenderer);
}

/**
 * Helper for `VuoSceneRenderer_render*`.
 */
extern "C" bool VuoSceneRenderer_renderInternal(VuoSceneRenderer sr, VuoGlContext glContext, GLuint *outputTexture, GLenum target, GLuint imageGlInternalFormat, VuoMultisample multisample, GLuint *outputDepthTexture)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;

	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	static bool force2DDepth = true;
	static dispatch_once_t once = 0;
	dispatch_once(&once, ^{
		// If the user set the `force2DDepth` preference, use it.
		Boolean overridden = false;
		force2DDepth = (int)CFPreferencesGetAppIntegerValue(CFSTR("force2DDepth"), CFSTR("org.vuo.Editor"), &overridden);

		if (!overridden)
		{
			// https://b33p.net/kosada/node/13485
			// https://b33p.net/kosada/node/13896
			// On some NVIDIA GPUs, the texture targets for the color and depth buffers need to match
			// (otherwise it sporadically crashes, or if a window is added via livecoding, it renders the scene with the wrong colors).
			// On AMD 7970 and AMD M370X, the depth texture target needs to always be GL_TEXTURE_2D
			// (otherwise it throws GL_INVALID_OPERATION).
			const char *renderer = (const char *)glGetString(GL_RENDERER);
			if (strcmp(renderer, "NVIDIA GeForce GT 650M OpenGL Engine") == 0
			 || strcmp(renderer, "NVIDIA GeForce 9400M OpenGL Engine") == 0)
				force2DDepth = false;
			else
				force2DDepth = true;
		}

		VDebugLog("force2DDepth = %d", force2DDepth);
	});

	unsigned char colorBytesPerPixel = VuoGlTexture_getBytesPerPixel(imageGlInternalFormat, GL_BGRA);
	unsigned long requiredBytes = sceneRenderer->viewportWidth * sceneRenderer->viewportHeight * colorBytesPerPixel;
	if (outputDepthTexture)
		requiredBytes += sceneRenderer->viewportWidth * sceneRenderer->viewportHeight * VuoGlTexture_getBytesPerPixel(GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);

	int supportedSamples = VuoGlContext_getMaximumSupportedMultisampling(glContext);
	multisample = (VuoMultisample)MIN(multisample, supportedSamples);
	bool actuallyMultisampling = multisample > 1;

	// See https://b33p.net/kosada/node/12030
	float fudge = 2.5;
	if (multisample)
		requiredBytes += requiredBytes * multisample * fudge;

	unsigned long maximumTextureBytes = VuoGlTexture_getMaximumTextureBytes(glContext);
	if (maximumTextureBytes > 0 && requiredBytes > maximumTextureBytes)
	{
		VUserLog("Not enough graphics memory for a %dx%d (%d bytes/pixel * %d sample) render%s.  Requires %lu MB, have %lu MB.",
				 sceneRenderer->viewportWidth, sceneRenderer->viewportHeight,
				 colorBytesPerPixel, multisample==0?1:multisample,
				 outputDepthTexture ? " with depth buffer" : "",
				 requiredBytes/1024/1024, maximumTextureBytes/1024/1024);
		return false;
	}


	// Create a new GL Texture Object and Framebuffer Object.
	if (!*outputTexture)
		*outputTexture = VuoGlTexturePool_use(glContext, imageGlInternalFormat, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight, GL_BGRA);

	if (outputDepthTexture)
		*outputDepthTexture = VuoGlTexturePool_use(glContext, GL_DEPTH_COMPONENT, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight, GL_DEPTH_COMPONENT);

	if (!*outputTexture || (outputDepthTexture && !*outputDepthTexture))
		return false;

	GLuint outputFramebuffer;
	glGenFramebuffers(1, &outputFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, outputFramebuffer);


	// If multisampling, create high-res intermediate renderbuffers.
	// Otherwise, directly bind the textures to the framebuffer.
	GLuint renderBuffer=0;
	GLuint renderDepthBuffer=0;
	if (actuallyMultisampling)
	{
		glGenRenderbuffersEXT(1, &renderBuffer);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderBuffer);
//		VLog("glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, %d, %s, %d, %d);", multisample, VuoGl_stringForConstant(imageGlInternalFormat), sceneRenderer->viewportWidth, sceneRenderer->viewportHeight);
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, multisample, imageGlInternalFormat, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER_EXT, renderBuffer);

		if (outputDepthTexture)
		{
			glGenRenderbuffersEXT(1, &renderDepthBuffer);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderDepthBuffer);
			glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, multisample, GL_DEPTH_COMPONENT, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER_EXT, renderDepthBuffer);
		}
	}
	else
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, *outputTexture, 0);
		if (outputDepthTexture)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, force2DDepth ? GL_TEXTURE_2D : target, *outputDepthTexture, 0);
	}


	// Render.
	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	{
		glViewport(0, 0, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight);
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		VuoSceneRenderer_draw(sceneRenderer);
	}


	// If multisampling, resolve the renderbuffers into standard textures by blitting them.
	if (actuallyMultisampling)
	{
		GLuint outputFramebuffer2;
		glGenFramebuffers(1, &outputFramebuffer2);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, outputFramebuffer2);

		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, *outputTexture, 0);
			if (outputDepthTexture)
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, force2DDepth ? GL_TEXTURE_2D : target, *outputDepthTexture, 0);

			glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, outputFramebuffer);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glBlitFramebufferEXT(0, 0, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight,
								 0, 0, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight,
								 GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
								 GL_NEAREST);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, 0, 0);
			if (outputDepthTexture)
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, force2DDepth ? GL_TEXTURE_2D : target, 0, 0);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &outputFramebuffer2);

		glDeleteRenderbuffersEXT(1, &renderBuffer);
		if (outputDepthTexture)
			glDeleteRenderbuffersEXT(1, &renderDepthBuffer);
	}
	else
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, 0, 0);
		if (outputDepthTexture)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, force2DDepth ? GL_TEXTURE_2D : target, 0, 0);
	}


	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &outputFramebuffer);

	// Make sure the render commands are actually submitted before we release the semaphore,
	// since the textures we're using might immediately be recycled (if the rootSceneObject is released).
	glFlushRenderAPPLE();

	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);

	return true;
}

/**
 * Renders the scene onto `image` and optionally `depthImage`.
 */
void VuoSceneRenderer_renderToImage(VuoSceneRenderer sr, VuoImage *image, VuoImageColorDepth imageColorDepth, VuoMultisample multisample, VuoImage *depthImage)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;
	GLuint imageGlInternalFormat = VuoImageColorDepth_getGlInternalFormat(GL_BGRA, imageColorDepth);
	__block GLuint texture = 0;
	__block GLuint depthTexture = 0;
	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
	if (!VuoSceneRenderer_renderInternal(sr, cgl_ctx, &texture, GL_TEXTURE_2D, imageGlInternalFormat, multisample, depthImage ? &depthTexture : NULL))
	{
		*image = NULL;
		if (depthImage)
			*depthImage = NULL;
	}
	});

	*image = VuoImage_make(texture, imageGlInternalFormat, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight);
	if (depthImage)
		*depthImage = VuoImage_make(depthTexture, GL_DEPTH_COMPONENT, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight);
}

/**
 * Renders the scene onto an IOSurface.
 */
VuoIoSurface VuoSceneRenderer_renderToIOSurface(VuoSceneRenderer sr, VuoImageColorDepth imageColorDepth, VuoMultisample multisample, bool includeDepthBuffer)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;
	GLuint imageGlInternalFormat = VuoImageColorDepth_getGlInternalFormat(GL_BGRA, imageColorDepth);
	__block GLuint texture = 0;
	__block GLuint depthTexture = 0;
	__block bool renderSucceeded;
	__block VuoIoSurface vis;
	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
		vis = VuoIoSurfacePool_use(cgl_ctx, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight, &texture);
		renderSucceeded = VuoSceneRenderer_renderInternal(sr, cgl_ctx, &texture, GL_TEXTURE_RECTANGLE_ARB, imageGlInternalFormat, multisample, includeDepthBuffer ? &depthTexture : NULL);
	});
	if (!renderSucceeded)
	{
		VuoIoSurfacePool_disuse(vis);
		return NULL;
	}

	if (includeDepthBuffer)
	{
		VuoImage depthImage = VuoImage_make(depthTexture, GL_DEPTH_COMPONENT, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight);
		VuoRetain(depthImage);
		VuoRelease(depthImage);
	}

	return vis;
}
