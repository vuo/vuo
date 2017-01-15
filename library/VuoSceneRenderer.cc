/**
 * @file
 * VuoSceneRenderer implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <list>
#include <map>
#include <string>

#include "VuoSceneRenderer.h"
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
						 "VuoGLContext"
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

/**
 * Work around apparent GL driver bug, wherein
 * attempting to simultaneously bind the same buffer
 * to multiple VAOs on separate contexts causes a crash.
 * (Try running CompareCameras.vuo without this.)
 *
 * Also, using VuoSceneObjectRenderer and VuoSceneRender simultaneously seems to lead to crashes,
 * so we also use this semaphore to serialize OpenGL Transform Feedback.
 * https://b33p.net/kosada/node/8498
 */
dispatch_semaphore_t VuoSceneRenderer_vertexArraySemaphore;
/**
 * Initialize @c VuoSceneRenderer_vertexArraySemaphore.
 */
static void __attribute__((constructor)) VuoSceneRenderer_init()
{
	VuoSceneRenderer_vertexArraySemaphore = dispatch_semaphore_create(1);
}

typedef std::list<GLuint> VuoSceneRendererVAOList;	///< The VAO for each VuoSubmesh
typedef std::pair<void *, void *> VuoSceneRendererMeshShader;	///< A VuoMesh/VuoShader combination
typedef std::map<VuoSceneRendererMeshShader, VuoSceneRendererVAOList> VuoSceneRendererMeshShaderVAOs;	///< The VAO for each VuoMesh/VuoShader combination

/**
 * GL Objects corresponding with a VuoSceneObject instance.
 */
class VuoSceneRendererInternal_object
{
public:
	VuoSceneRendererVAOList meshItems;	///< VAOs for each submesh.

	VuoMesh overrideMesh;	///< If non-null, this mesh is rendered instead of the actual object's mesh.
	enum
	{
		RealSize_Inherit,
		RealSize_True,
	} overrideIsRealSize;	///< If not Inherit, this overrides the actual object's isRealSize property.
	VuoShader overrideShader;	///< If non-null, this shader is rendered instead of the actual object's shader.

	std::list<VuoSceneRendererInternal_object> childObjects;	///< Hierarchy
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

	VuoGlContext glContext;

#ifdef PROFILE
	std::list<VuoProfileEntry> profileTimes;	///< How long each object took to render.
#endif
};

void VuoSceneRenderer_destroy(VuoSceneRenderer sr);

/**
 * Configures OpenGL state for the specified context.
 *
 * @threadAnyGL
 */
static void VuoSceneRenderer_prepareContext(CGLContextObj cgl_ctx)
{
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
}

static void VuoSceneRenderer_uploadSceneObject(VuoSceneRendererInternal *sceneRenderer, VuoSceneObject so, VuoSceneRendererInternal_object *soi, VuoGlContext glContext, bool cache);

#if LIBRARY_PREMIUM_AVAILABLE
#include "premium/VuoSceneRendererPremium.h"
#endif

/**
 * Creates a reference-counted object for rendering a scenegraph.
 *
 * @threadAny
 */
VuoSceneRenderer VuoSceneRenderer_make(VuoGlContext glContext, float backingScaleFactor)
{
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
	sceneRenderer->glContext = glContext;
	sceneRenderer->backingScaleFactor = backingScaleFactor;

	sceneRenderer->vignetteShader = NULL;
#if LIBRARY_PREMIUM_AVAILABLE
	VuoSceneRendererPremium_init(sceneRenderer);
#endif

	VuoSceneRenderer_prepareContext((CGLContextObj)glContext);

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
	if (camera.type == VuoSceneObjectType_PerspectiveCamera)
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
	else if (camera.type == VuoSceneObjectType_StereoCamera)
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
	else if (camera.type == VuoSceneObjectType_OrthographicCamera)
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
	else if (camera.type == VuoSceneObjectType_FisheyeCamera)
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
 * Draws @c so (using the uploaded object names in @c soi).
 * Does not traverse child objects.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
static void VuoSceneRenderer_drawSceneObject(VuoSceneObject so, VuoSceneRendererInternal_object *soi, float projectionMatrix[16], float modelviewMatrix[16], VuoSceneRendererInternal *sceneRenderer)
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

	CGLContextObj cgl_ctx = (CGLContextObj)sceneRenderer->glContext;

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

	// All VuoSubmeshes are assumed to have the same elementAssemblyMethod.
	VuoGlProgram program;
	if (!VuoShader_activate(so.shader, so.mesh->submeshes[0].elementAssemblyMethod, sceneRenderer->glContext, &program))
	{
		VUserLog("Shader activation failed.");
		return;
	}

	{
		GLint projectionMatrixUniform = VuoGlProgram_getUniformLocation(program, "projectionMatrix");
		glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, projectionMatrix);

		GLint useFisheyeProjectionUniform = VuoGlProgram_getUniformLocation(program, "useFisheyeProjection");
		if (useFisheyeProjectionUniform != -1)
			glUniform1i(useFisheyeProjectionUniform, sceneRenderer->camera.type == VuoSceneObjectType_FisheyeCamera);

		GLint modelviewMatrixUniform = VuoGlProgram_getUniformLocation(program, "modelviewMatrix");

		if (so.isRealSize)
		{
			float billboardMatrix[16];
			VuoTransform_getBillboardMatrix(image->pixelsWide, image->pixelsHigh, modelviewMatrix[12], modelviewMatrix[13], sceneRenderer->viewportWidth, sceneRenderer->viewportHeight, billboardMatrix);
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
			glUniform4f(ambientColorUniform, sceneRenderer->ambientColor.r, sceneRenderer->ambientColor.g, sceneRenderer->ambientColor.b, sceneRenderer->ambientColor.a);
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
				glUniform4f(colorUniform, pointLight.lightColor.r, pointLight.lightColor.g, pointLight.lightColor.b, pointLight.lightColor.a);

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
				glUniform4f(colorUniform, spotLight.lightColor.r, spotLight.lightColor.g, spotLight.lightColor.b, spotLight.lightColor.a);

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

		if (so.shader->isTransparent)
			glDepthMask(false);

		if (so.blendMode != VuoBlendMode_Normal)
		{
			glDisable(GL_DEPTH_TEST);

			if (so.blendMode == VuoBlendMode_Multiply)
			{
				glBlendFunc(GL_DST_COLOR, GL_ZERO);
				glBlendEquation(GL_FUNC_ADD);
			}
			else if (so.blendMode == VuoBlendMode_DarkerComponent)
			{
				glBlendFunc(GL_ONE, GL_ONE);
				glBlendEquationSeparate(GL_MIN, GL_FUNC_ADD);
			}
			else if (so.blendMode == VuoBlendMode_LighterComponent)
			{
				glBlendFunc(GL_ONE, GL_ONE);
				glBlendEquationSeparate(GL_MAX, GL_FUNC_ADD);
			}
			else if (so.blendMode == VuoBlendMode_LinearDodge)
			{
				glBlendFunc(GL_ONE, GL_ONE);
				glBlendEquation(GL_FUNC_ADD);
			}
			else if (so.blendMode == VuoBlendMode_Subtract)
			{
				glBlendFuncSeparate(GL_ONE, GL_ONE, GL_DST_ALPHA, GL_ZERO);
				glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD);
			}
		}

		unsigned int i = 0;
		dispatch_semaphore_wait(VuoSceneRenderer_vertexArraySemaphore, DISPATCH_TIME_FOREVER);
		for (VuoSceneRendererVAOList::iterator vi = soi->meshItems.begin(); vi != soi->meshItems.end(); ++vi, ++i)
		{
			glBindVertexArray(*vi);

			VuoSubmesh submesh = so.mesh->submeshes[i];
			GLenum mode = VuoSubmesh_getGlMode(submesh);

			GLint hasTextureCoordinatesUniform = VuoGlProgram_getUniformLocation(program, "hasTextureCoordinates");
			if (hasTextureCoordinatesUniform != -1)
				glUniform1i(hasTextureCoordinatesUniform, submesh.glUpload.textureCoordinateOffset != NULL);

			GLint primitiveHalfSizeUniform = VuoGlProgram_getUniformLocation(program, "primitiveHalfSize");
			if (primitiveHalfSizeUniform != -1)
				glUniform1f(primitiveHalfSizeUniform, submesh.primitiveSize/2);

			if (submesh.faceCullingMode == GL_NONE || so.shader->isTransparent)
				glDisable(GL_CULL_FACE);
			else
			{
				glEnable(GL_CULL_FACE);
				glCullFace(submesh.faceCullingMode);
			}

			if (submesh.elementCount)
				glDrawElements(mode, (GLsizei)submesh.elementCount, GL_UNSIGNED_INT, (void*)0);
			else
				glDrawArrays(mode, 0, submesh.vertexCount);

			glBindVertexArray(0);
		}
		dispatch_semaphore_signal(VuoSceneRenderer_vertexArraySemaphore);

		if (so.blendMode != VuoBlendMode_Normal)
		{
			glEnable(GL_DEPTH_TEST);
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
			glBlendEquation(GL_FUNC_ADD);
		}

		if (so.shader->isTransparent)
			glDepthMask(true);
	}
	VuoShader_deactivate(so.shader, so.mesh->submeshes[0].elementAssemblyMethod, sceneRenderer->glContext);

#ifdef _PROFILE
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
 * Draws @c so and its child objects.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
static void VuoSceneRenderer_drawSceneObjectsRecursively(VuoSceneObject so, VuoSceneRendererInternal_object *soi, float projectionMatrix[16], float modelviewMatrix[16], VuoSceneRendererInternal *sceneRenderer)
{
	float localModelviewMatrix[16];
	VuoTransform_getMatrix(so.transform, localModelviewMatrix);
	float compositeModelviewMatrix[16];
	VuoTransform_multiplyMatrices4x4(localModelviewMatrix, modelviewMatrix, compositeModelviewMatrix);
	VuoSceneRenderer_drawSceneObject(so, soi, projectionMatrix, compositeModelviewMatrix, sceneRenderer);
//	VuoSceneRenderer_drawElement(so, projectionMatrix, compositeModelviewMatrix, sceneRenderer->glContext, 0, .08f);	// Normals
//	VuoSceneRenderer_drawElement(so, projectionMatrix, compositeModelviewMatrix, sceneRenderer->glContext, 1, .08f);	// Tangents
//	VuoSceneRenderer_drawElement(so, projectionMatrix, compositeModelviewMatrix, sceneRenderer->glContext, 2, .08f);	// Bitangents

	unsigned int i = 1;
	for (std::list<VuoSceneRendererInternal_object>::iterator oi = soi->childObjects.begin(); oi != soi->childObjects.end(); ++oi, ++i)
	{
		VuoSceneObject childObject = VuoListGetValue_VuoSceneObject(so.childObjects, i);
		VuoSceneRenderer_drawSceneObjectsRecursively(childObject, &(*oi), projectionMatrix, compositeModelviewMatrix, sceneRenderer);
	}
}

//static void VuoSceneRenderer_drawLights(VuoSceneRendererInternal *sceneRenderer);
static void VuoSceneRenderer_cleanupSceneObjectsRecursively(VuoSceneObject so, VuoSceneRendererInternal_object *soi, VuoGlContext glContext);
static void VuoSceneRenderer_cleanupMeshShaderItems(VuoSceneRendererInternal *sceneRenderer);
static void VuoSceneRenderer_uploadSceneObjectsRecursively(VuoSceneRendererInternal *sceneRenderer, VuoSceneObject so, VuoSceneRendererInternal_object *soi, VuoGlContext glContext);

/**
 * Uploads the scene to the GPU (if it's changed), and renders it.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_draw(VuoSceneRenderer sr)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;
	CGLContextObj cgl_ctx = (CGLContextObj)sceneRenderer->glContext;

	if (!sceneRenderer->rootSceneObjectPendingValid && !sceneRenderer->rootSceneObjectValid)
		return;

	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);

	if (sceneRenderer->rootSceneObjectPendingUpdated)
	{
		// Release the old scenegraph.
		if (sceneRenderer->rootSceneObjectValid)
		{
			VuoSceneRenderer_cleanupSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, sceneRenderer->glContext);
			VuoSceneRenderer_cleanupMeshShaderItems(sceneRenderer);
			VuoSceneObject_release(sceneRenderer->rootSceneObject);
			VuoRelease(sceneRenderer->pointLights);
			VuoRelease(sceneRenderer->spotLights);
		}

		// Upload the new scenegraph.
		sceneRenderer->rootSceneObject = sceneRenderer->rootSceneObjectPending;
		VuoSceneObject_retain(sceneRenderer->rootSceneObject);
		VuoSceneRenderer_uploadSceneObjectsRecursively(sceneRenderer, sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, sceneRenderer->glContext);

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

	float localModelviewMatrix[16];
	VuoTransform_getMatrix(VuoTransform_makeIdentity(), localModelviewMatrix);
	VuoSceneRenderer_drawSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, sceneRenderer->projectionMatrix, localModelviewMatrix, sceneRenderer);

//	VuoSceneRenderer_drawLights(sceneRenderer);


#if LIBRARY_PREMIUM_AVAILABLE
	if (sceneRenderer->camera.type == VuoSceneObjectType_FisheyeCamera)
		VuoSceneRendererPremium_drawVignette(sceneRenderer);
#endif


	// Make sure the render commands actually execute before we release the semaphore,
	// since the textures we're using might immediately be recycled (if the rootSceneObject is released).
	glFlushRenderAPPLE();

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

	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);
}

extern "C"
{
void VuoSubmeshMesh_download(VuoSubmesh *submesh);
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
		VuoSubmeshMesh_download(&submesh);

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

	soi->overrideMesh = VuoMesh_makeQuadWithoutNormals();
	VuoRetain(soi->overrideMesh);

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

	if (so.type == VuoSceneObjectType_Text && so.text && so.text[0] != 0)
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
		soi->meshItems = i->second;
		return;
	}


	// …otherwise, create one.

	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	// Fetch the shader's attribute locations.
	// All VuoSubmeshes are assumed to have the same elementAssemblyMethod.
	GLint positionAttribute, normalAttribute, tangentAttribute, bitangentAttribute, textureCoordinateAttribute;
	VuoShader_getAttributeLocations(so.shader, so.mesh->submeshes[0].elementAssemblyMethod, glContext, &positionAttribute, &normalAttribute, &tangentAttribute, &bitangentAttribute, &textureCoordinateAttribute);

	// For each mesh item in the sceneobject...
	dispatch_semaphore_wait(VuoSceneRenderer_vertexArraySemaphore, DISPATCH_TIME_FOREVER);
	for (unsigned long i = 0; i < so.mesh->submeshCount; ++i)
	{
		VuoSubmesh meshItem = so.mesh->submeshes[i];
		GLuint vertexArray;


		// Create a Vertex Array Object, to store this sceneobject's vertex array bindings.
		glGenVertexArrays(1, &vertexArray);
		glBindVertexArray(vertexArray);


		// Bind the combined buffer to the Vertex Array Object.
		glBindBuffer(GL_ARRAY_BUFFER, so.mesh->submeshes[i].glUpload.combinedBuffer);


		// Populate the Vertex Array Object with the various vertex attributes.
		int bufferCount = 0;
		++bufferCount; // position
		if (meshItem.glUpload.normalOffset)
			++bufferCount;
		if (meshItem.glUpload.tangentOffset)
			++bufferCount;
		if (meshItem.glUpload.bitangentOffset)
			++bufferCount;
		if (meshItem.glUpload.textureCoordinateOffset)
			++bufferCount;

		glEnableVertexAttribArray((GLuint)positionAttribute);
		glVertexAttribPointer((GLuint)positionAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d)*bufferCount, (void*)0);

		if (meshItem.glUpload.normalOffset && normalAttribute>=0)
		{
			glEnableVertexAttribArray((GLuint)normalAttribute);
			glVertexAttribPointer((GLuint)normalAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d)*bufferCount, so.mesh->submeshes[i].glUpload.normalOffset);
		}

		if (meshItem.glUpload.tangentOffset && tangentAttribute>=0)
		{
			glEnableVertexAttribArray((GLuint)tangentAttribute);
			glVertexAttribPointer((GLuint)tangentAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d)*bufferCount, so.mesh->submeshes[i].glUpload.tangentOffset);
		}

		if (meshItem.glUpload.bitangentOffset && bitangentAttribute>=0)
		{
			glEnableVertexAttribArray((GLuint)bitangentAttribute);
			glVertexAttribPointer((GLuint)bitangentAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d)*bufferCount, so.mesh->submeshes[i].glUpload.bitangentOffset);
		}

		if (meshItem.glUpload.textureCoordinateOffset && textureCoordinateAttribute>=0)
		{
			glEnableVertexAttribArray((GLuint)textureCoordinateAttribute);
			glVertexAttribPointer((GLuint)textureCoordinateAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d)*bufferCount, so.mesh->submeshes[i].glUpload.textureCoordinateOffset);
		}


		// Bind the Element Buffer to the Vertex Array Object
		if (so.mesh->submeshes[i].elementCount)
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, so.mesh->submeshes[i].glUpload.elementBuffer);


		glBindVertexArray(0);

		soi->meshItems.push_back(vertexArray);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	dispatch_semaphore_signal(VuoSceneRenderer_vertexArraySemaphore);

	if (cache)
		sceneRenderer->meshShaderItems[meshShader] = soi->meshItems;
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
		return;

	unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
	for (unsigned long i = 1; i <= childObjectCount; ++i)
	{
		VuoSceneObject childObject = VuoListGetValue_VuoSceneObject(so.childObjects, i);
		VuoSceneRendererInternal_object childObjectInternal;
		VuoSceneRenderer_uploadSceneObjectsRecursively(sceneRenderer, childObject, &childObjectInternal, glContext);
		soi->childObjects.push_back(childObjectInternal);
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
	unsigned int i = 1;
	for (std::list<VuoSceneRendererInternal_object>::iterator oi = soi->childObjects.begin(); oi != soi->childObjects.end(); ++oi, ++i)
	{
		VuoSceneObject childObject = VuoListGetValue_VuoSceneObject(so.childObjects, i);
		VuoSceneRenderer_cleanupSceneObjectsRecursively(childObject, &(*oi), glContext);
	}

	soi->meshItems.clear();

	if (soi->overrideMesh)
		VuoRelease(soi->overrideMesh);
	if (soi->overrideShader)
		VuoRelease(soi->overrideShader);

	soi->childObjects.clear();
}

/**
 * Releases the GPU objects created by @c VuoSceneRenderer_uploadSceneObjectsRecursively.
 */
static void VuoSceneRenderer_cleanupMeshShaderItems(VuoSceneRendererInternal *sceneRenderer)
{
	CGLContextObj cgl_ctx = (CGLContextObj)sceneRenderer->glContext;

	dispatch_semaphore_wait(VuoSceneRenderer_vertexArraySemaphore, DISPATCH_TIME_FOREVER);
	for (VuoSceneRendererMeshShaderVAOs::iterator msi = sceneRenderer->meshShaderItems.begin(); msi != sceneRenderer->meshShaderItems.end(); ++msi)
		for (VuoSceneRendererVAOList::iterator i = msi->second.begin(); i != msi->second.end(); ++i)
			glDeleteVertexArrays(1, &(*i));
	dispatch_semaphore_signal(VuoSceneRenderer_vertexArraySemaphore);

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

	VuoShader_cleanupContext(sceneRenderer->glContext);

	if (sceneRenderer->rootSceneObjectPendingValid)
		VuoSceneObject_release(sceneRenderer->rootSceneObjectPending);

	if (sceneRenderer->rootSceneObjectValid)
	{
		VuoSceneRenderer_cleanupSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, sceneRenderer->glContext);
		VuoSceneRenderer_cleanupMeshShaderItems(sceneRenderer);
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
 * Creates an OpenGL Framebuffer Object, and uses it to render the scene to @c image and @c depthImage.
 */
void VuoSceneRenderer_renderToImage(VuoSceneRenderer sr, VuoImage *image, VuoImageColorDepth imageColorDepth, VuoMultisample multisample, VuoImage *depthImage)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;

	CGLContextObj cgl_ctx = (CGLContextObj)sceneRenderer->glContext;

	GLuint imageGlInternalFormat = VuoImageColorDepth_getGlInternalFormat(GL_BGRA, imageColorDepth);

	// Create a new GL Texture Object and Framebuffer Object.
	GLuint outputTexture = VuoGlTexturePool_use(sceneRenderer->glContext, imageGlInternalFormat, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight, GL_BGRA);

	GLuint outputDepthTexture=0;
	if (depthImage)
		outputDepthTexture = VuoGlTexturePool_use(sceneRenderer->glContext, GL_DEPTH_COMPONENT, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight, GL_DEPTH_COMPONENT);

	GLuint outputFramebuffer;
	glGenFramebuffers(1, &outputFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, outputFramebuffer);


	// If multisampling, create high-res intermediate renderbuffers.
	// Otherwise, directly bind the textures to the framebuffer.
	GLuint renderBuffer=0;
	GLuint renderDepthBuffer=0;
	bool actuallyMultisampling = (multisample > 1 && VuoGlContext_isMultisamplingFunctional(sceneRenderer->glContext));
	if (actuallyMultisampling)
	{
		glGenRenderbuffersEXT(1, &renderBuffer);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderBuffer);
//		VLog("glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, %d, %s, %d, %d);", multisample, VuoGl_stringForConstant(imageGlInternalFormat), sceneRenderer->viewportWidth, sceneRenderer->viewportHeight);
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, multisample, imageGlInternalFormat, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER_EXT, renderBuffer);

		if (depthImage)
		{
			glGenRenderbuffersEXT(1, &renderDepthBuffer);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderDepthBuffer);
			glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, multisample, GL_DEPTH_COMPONENT, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER_EXT, renderDepthBuffer);
		}
	}
	else
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);
		if (depthImage)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, outputDepthTexture, 0);
	}


	// Render.
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
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);
			if (depthImage)
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, outputDepthTexture, 0);

			glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, outputFramebuffer);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glBlitFramebufferEXT(0, 0, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight,
								 0, 0, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight,
								 GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
								 GL_NEAREST);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
			if (depthImage)
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &outputFramebuffer2);

		glDeleteRenderbuffersEXT(1, &renderBuffer);
		if (depthImage)
			glDeleteRenderbuffersEXT(1, &renderDepthBuffer);
	}
	else
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		if (depthImage)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
	}


	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &outputFramebuffer);

	*image = VuoImage_make(outputTexture, imageGlInternalFormat, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight);
	if (depthImage)
		*depthImage = VuoImage_make(outputDepthTexture, GL_DEPTH_COMPONENT, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight);
}
