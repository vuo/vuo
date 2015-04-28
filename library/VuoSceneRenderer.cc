/**
 * @file
 * VuoSceneRenderer implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <list>

#include "VuoSceneRenderer.h"
#include "VuoSceneObject.h"
#include "VuoGlPool.h"

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
						 "VuoGLContext"
					 ]
				 });
#endif
}

/**
 * GL Objects corresponding with a VuoVertices instance.
 */
struct VuoSceneRendererInternal_vertices
{
	GLuint vertexArray;

	GLuint combinedBuffer;
	GLuint combinedBufferSize;

	GLuint elementBuffer;
	GLuint elementBufferSize;
};

/**
 * GL Objects corresponding with a VuoSceneObject instance.
 */
class VuoSceneRendererInternal_object
{
public:
	std::list<VuoSceneRendererInternal_vertices> vertices;
	std::list<VuoSceneRendererInternal_object> childObjects;
};

/**
 * Internal state data for a VuoSceneRenderer instance.
 */
class VuoSceneRendererInternal
{
public:
	dispatch_semaphore_t scenegraphSemaphore; ///< Serializes access to other data in this structure.
	bool scenegraphValid;
	bool needToRegenerateProjectionMatrix;
	unsigned int viewportWidth;
	unsigned int viewportHeight;

	VuoSceneObject rootSceneObject;
	VuoSceneRendererInternal_object rootSceneObjectInternal;

	float projectionMatrix[16]; ///< Column-major 4x4 matrix
	VuoText cameraName;
	VuoSceneObject camera;

	VuoColor ambientColor;
	float ambientBrightness;
	VuoList_VuoSceneObject pointLights;
	VuoList_VuoSceneObject spotLights;

	VuoGlContext glContext;
};

void VuoSceneRenderer_destroy(VuoSceneRenderer sr);

/**
 * Configures OpenGL state for the specified context.
 *
 * @threadAnyGL
 */
static void VuoSceneRenderer_prepareContext(CGLContextObj cgl_ctx)
{
	glEnable(GL_MULTISAMPLE);

	glEnable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

/**
 * Creates a reference-counted object for rendering a scenegraph.
 *
 * @threadAny
 */
VuoSceneRenderer VuoSceneRenderer_make(VuoGlContext glContext)
{
	VuoSceneRendererInternal *sceneRenderer = new VuoSceneRendererInternal;
	VuoRegister(sceneRenderer, VuoSceneRenderer_destroy);

	sceneRenderer->scenegraphSemaphore = dispatch_semaphore_create(1);
	sceneRenderer->scenegraphValid = false;
	sceneRenderer->needToRegenerateProjectionMatrix = false;
	sceneRenderer->cameraName = NULL;
	sceneRenderer->glContext = glContext;

	VuoSceneRenderer_prepareContext((CGLContextObj)glContext);

	return (VuoSceneRenderer)sceneRenderer;
}

void VuoSceneRenderer_regenerateProjectionMatrixInternal(VuoSceneRendererInternal *sceneRenderer);

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
		if (sceneRenderer->scenegraphValid)
		{
			bool foundCamera;
			camera = VuoSceneObject_findCamera(sceneRenderer->rootSceneObject, sceneRenderer->cameraName, &foundCamera);

			if (!foundCamera)
			{
				VuoSceneObject_retain(camera);
				VuoSceneObject_release(camera);

				// Search again, and this time just pick the first camera we find.
				camera = VuoSceneObject_findCamera(sceneRenderer->rootSceneObject, "", &foundCamera);
			}
		}
		else
			camera = VuoSceneObject_makeDefaultCamera();
	}


	// Build a projection matrix for a camera located at the origin, facing along the -z axis.
	float aspectRatio = (float)sceneRenderer->viewportWidth/(float)sceneRenderer->viewportHeight;
	if (camera.cameraType == VuoSceneObject_PerspectiveCamera)
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
	else if (camera.cameraType == VuoSceneObject_OrthographicCamera)
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
	else
		VLog("Unknown cameraType %d", camera.cameraType);


	// Position the camera (translate then rotate the scene by the inverse of the camera transformation).

	// VuoTransform_getMatrix() performs the transformation in this order: rotate, scale, translate.
	// So we need to do this in multiple steps, in order to apply the translation first.

	float rotationMatrix[16];
	VuoTransform_getMatrix(camera.transform, (float *)rotationMatrix);
	// Remove the translation.
	rotationMatrix[12] = rotationMatrix[13] = rotationMatrix[14] = 0;

	float outputMatrix[16];
	VuoTransform_multiplyMatrices4x4((float *)rotationMatrix, sceneRenderer->projectionMatrix, (float *)&outputMatrix);

	float translationMatrix[16];
	VuoTransform_getMatrix(VuoTransform_makeEuler(
							   VuoPoint3d_multiply(camera.transform.translation,-1),
							   VuoPoint3d_make(0,0,0),
							   VuoPoint3d_make(1,1,1)),
						   (float *)translationMatrix);

	float outputMatrix2[16];
	VuoTransform_multiplyMatrices4x4((float *)translationMatrix, (float *)outputMatrix, (float *)&outputMatrix2);

	VuoTransform_copyMatrix4x4((float *)outputMatrix2, sceneRenderer->projectionMatrix);

	VuoSceneObject_release(sceneRenderer->camera);
	sceneRenderer->camera = camera;
	VuoSceneObject_retain(sceneRenderer->camera);
}

/**
 * Draws @c so (using the uploaded object names in @c soi).
 * Does not traverse child objects.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_drawSceneObject(VuoSceneObject so, VuoSceneRendererInternal_object *soi, float projectionMatrix[16], float modelviewMatrix[16], VuoSceneRendererInternal *sceneRenderer)
{
	if (!so.shader)
		return;

	dispatch_semaphore_wait(so.shader->lock, DISPATCH_TIME_FOREVER);
	CGLContextObj cgl_ctx = (CGLContextObj)sceneRenderer->glContext;

	glUseProgram(so.shader->glProgramName);
	{
		GLint projectionMatrixUniform = glGetUniformLocation(so.shader->glProgramName, "projectionMatrix");
		glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, projectionMatrix);

		GLint modelviewMatrixUniform = glGetUniformLocation(so.shader->glProgramName, "modelviewMatrix");

		if (so.isRealSize)
		{
			VuoImage image = VuoListGetValueAtIndex_VuoImage(so.shader->textures,1);
			float billboardMatrix[16];
			VuoTransform_getBillboardMatrix(image->pixelsWide, image->pixelsHigh, modelviewMatrix[12], modelviewMatrix[13], sceneRenderer->viewportWidth, sceneRenderer->viewportHeight, billboardMatrix);
			glUniformMatrix4fv(modelviewMatrixUniform, 1, GL_FALSE, billboardMatrix);
		}
		else
			glUniformMatrix4fv(modelviewMatrixUniform, 1, GL_FALSE, modelviewMatrix);

		GLint cameraPositionUniform = glGetUniformLocation(so.shader->glProgramName, "cameraPosition");
		if (cameraPositionUniform)
			glUniform3f(cameraPositionUniform, sceneRenderer->camera.transform.translation.x, sceneRenderer->camera.transform.translation.y, sceneRenderer->camera.transform.translation.z);

		GLint ambientColorUniform = glGetUniformLocation(so.shader->glProgramName, "ambientColor");
		if (ambientColorUniform != -1)
			glUniform4f(ambientColorUniform, sceneRenderer->ambientColor.r, sceneRenderer->ambientColor.g, sceneRenderer->ambientColor.b, sceneRenderer->ambientColor.a);
		GLint ambientBrightnessUniform = glGetUniformLocation(so.shader->glProgramName, "ambientBrightness");
		if (ambientBrightnessUniform != -1)
			glUniform1f(ambientBrightnessUniform, sceneRenderer->ambientBrightness);

		int pointLightCount = VuoListGetCount_VuoSceneObject(sceneRenderer->pointLights);
		GLint pointLightCountUniform = glGetUniformLocation(so.shader->glProgramName, "pointLightCount");
		if (pointLightCountUniform != -1)
		{
			glUniform1i(pointLightCountUniform, pointLightCount);

			for (int i=1; i<=pointLightCount; ++i)
			{
				VuoSceneObject pointLight = VuoListGetValueAtIndex_VuoSceneObject(sceneRenderer->pointLights, i);

				int uniformNameMaxLength = strlen("pointLights[15].brightness")+1;
				char *uniformName = (char *)malloc(uniformNameMaxLength);

				snprintf(uniformName, uniformNameMaxLength, "pointLights[%d].color", i-1);
				GLint colorUniform = glGetUniformLocation(so.shader->glProgramName, uniformName);
				glUniform4f(colorUniform, pointLight.lightColor.r, pointLight.lightColor.g, pointLight.lightColor.b, pointLight.lightColor.a);

				snprintf(uniformName, uniformNameMaxLength, "pointLights[%d].brightness", i-1);
				GLint brightnessUniform = glGetUniformLocation(so.shader->glProgramName, uniformName);
				glUniform1f(brightnessUniform, pointLight.lightBrightness);

				snprintf(uniformName, uniformNameMaxLength, "pointLights[%d].position", i-1);
				GLint positionUniform = glGetUniformLocation(so.shader->glProgramName, uniformName);
				glUniform3f(positionUniform, pointLight.transform.translation.x, pointLight.transform.translation.y, pointLight.transform.translation.z);

				snprintf(uniformName, uniformNameMaxLength, "pointLights[%d].range", i-1);
				GLint rangeUniform = glGetUniformLocation(so.shader->glProgramName, uniformName);
				glUniform1f(rangeUniform, pointLight.lightRange);

				snprintf(uniformName, uniformNameMaxLength, "pointLights[%d].sharpness", i-1);
				GLint sharpnessUniform = glGetUniformLocation(so.shader->glProgramName, uniformName);
				glUniform1f(sharpnessUniform, pointLight.lightSharpness);
			}
		}

		int spotLightCount = VuoListGetCount_VuoSceneObject(sceneRenderer->spotLights);
		GLint spotLightCountUniform = glGetUniformLocation(so.shader->glProgramName, "spotLightCount");
		if (spotLightCountUniform != -1)
		{
			glUniform1i(spotLightCountUniform, spotLightCount);

			for (int i=1; i<=spotLightCount; ++i)
			{
				VuoSceneObject spotLight = VuoListGetValueAtIndex_VuoSceneObject(sceneRenderer->spotLights, i);

				int uniformNameMaxLength = strlen("spotLights[15].brightness")+1;
				char *uniformName = (char *)malloc(uniformNameMaxLength);

				snprintf(uniformName, uniformNameMaxLength, "spotLights[%d].color", i-1);
				GLint colorUniform = glGetUniformLocation(so.shader->glProgramName, uniformName);
				glUniform4f(colorUniform, spotLight.lightColor.r, spotLight.lightColor.g, spotLight.lightColor.b, spotLight.lightColor.a);

				snprintf(uniformName, uniformNameMaxLength, "spotLights[%d].brightness", i-1);
				GLint brightnessUniform = glGetUniformLocation(so.shader->glProgramName, uniformName);
				glUniform1f(brightnessUniform, spotLight.lightBrightness);

				snprintf(uniformName, uniformNameMaxLength, "spotLights[%d].position", i-1);
				GLint positionUniform = glGetUniformLocation(so.shader->glProgramName, uniformName);
				glUniform3f(positionUniform, spotLight.transform.translation.x, spotLight.transform.translation.y, spotLight.transform.translation.z);

				snprintf(uniformName, uniformNameMaxLength, "spotLights[%d].direction", i-1);
				GLint directionUniform = glGetUniformLocation(so.shader->glProgramName, uniformName);
				VuoPoint3d direction = VuoTransform_getDirection(spotLight.transform);
				glUniform3f(directionUniform, direction.x, direction.y, direction.z);

				snprintf(uniformName, uniformNameMaxLength, "spotLights[%d].cone", i-1);
				GLint coneUniform = glGetUniformLocation(so.shader->glProgramName, uniformName);
				glUniform1f(coneUniform, spotLight.lightCone);

				snprintf(uniformName, uniformNameMaxLength, "spotLights[%d].range", i-1);
				GLint rangeUniform = glGetUniformLocation(so.shader->glProgramName, uniformName);
				glUniform1f(rangeUniform, spotLight.lightRange);

				snprintf(uniformName, uniformNameMaxLength, "spotLights[%d].sharpness", i-1);
				GLint sharpnessUniform = glGetUniformLocation(so.shader->glProgramName, uniformName);
				glUniform1f(sharpnessUniform, spotLight.lightSharpness);
			}
		}

		VuoShader_activateTextures(so.shader, cgl_ctx);

		unsigned int i = 1;
		for (std::list<VuoSceneRendererInternal_vertices>::iterator vi = soi->vertices.begin(); vi != soi->vertices.end(); ++vi, ++i)
		{
			glBindVertexArray((*vi).vertexArray);

			VuoVertices vertices = VuoListGetValueAtIndex_VuoVertices(so.verticesList, i);
			GLenum mode = GL_TRIANGLES;
			if (vertices.elementAssemblyMethod == VuoVertices_IndividualTriangles)
				mode = GL_TRIANGLES;
			else if (vertices.elementAssemblyMethod == VuoVertices_TriangleStrip)
				mode = GL_TRIANGLE_STRIP;
			else if (vertices.elementAssemblyMethod == VuoVertices_TriangleFan)
				mode = GL_TRIANGLE_FAN;
			else if (vertices.elementAssemblyMethod == VuoVertices_IndividualLines)
				mode = GL_LINES;
			else if (vertices.elementAssemblyMethod == VuoVertices_LineStrip)
				mode = GL_LINE_STRIP;
			else if (vertices.elementAssemblyMethod == VuoVertices_Points)
				mode = GL_POINTS;
			glDrawElements(mode, (GLsizei)vertices.elementCount, GL_UNSIGNED_INT, (void*)0);

			glBindVertexArray(0);
		}

		VuoShader_deactivateTextures(so.shader, cgl_ctx);
	}
	glUseProgram(0);
	dispatch_semaphore_signal(so.shader->lock);
}

void VuoSceneRenderer_drawElement(VuoSceneObject so, float projectionMatrix[16], float compositeModelviewMatrix[16], VuoGlContext glContext, int element, float length);

/**
 * Draws @c so and its child objects.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_drawSceneObjectsRecursively(VuoSceneObject so, VuoSceneRendererInternal_object *soi, float projectionMatrix[16], float modelviewMatrix[16], VuoSceneRendererInternal *sceneRenderer)
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
		VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
		VuoSceneRenderer_drawSceneObjectsRecursively(childObject, &(*oi), projectionMatrix, compositeModelviewMatrix, sceneRenderer);
	}
}

void VuoSceneRenderer_drawLights(VuoSceneRendererInternal *sceneRenderer);

/**
 * Renders the scene.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_draw(VuoSceneRenderer sr)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;
	CGLContextObj cgl_ctx = (CGLContextObj)sceneRenderer->glContext;

	if (!sceneRenderer->scenegraphValid)
		return;

	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);

	if (sceneRenderer->needToRegenerateProjectionMatrix)
	{
		VuoSceneRenderer_regenerateProjectionMatrixInternal(sceneRenderer);
		sceneRenderer->needToRegenerateProjectionMatrix = false;
	}

	float localModelviewMatrix[16];
	VuoTransform_getMatrix(VuoTransform_makeIdentity(), localModelviewMatrix);
	VuoSceneRenderer_drawSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, sceneRenderer->projectionMatrix, localModelviewMatrix, sceneRenderer);

//	VuoSceneRenderer_drawLights(sceneRenderer);

	// Make sure the render commands actually execute before we release the semaphore,
	// since the textures we're using might immediately be recycled (if the rootSceneObject is released).
	glFlushRenderAPPLE();

	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);
}

/**
 * Draws all vertex normals in @c so.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_drawElement(VuoSceneObject so, float projectionMatrix[16], float compositeModelviewMatrix[16], VuoGlContext glContext, int element, float length)	// TODO Enum type for element to debug
{
	if (!so.verticesList)
		return;

	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	unsigned long verticesListCount = VuoListGetCount_VuoVertices(so.verticesList);

	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projectionMatrix);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(compositeModelviewMatrix);

	glBegin(GL_LINES);
	for (unsigned int i = 1; i <= verticesListCount; i++)
	{
		VuoVertices vertices = VuoListGetValueAtIndex_VuoVertices(so.verticesList, i);

		for(unsigned int m = 0; m < vertices.vertexCount; m++)
		{
			VuoPoint4d v = vertices.positions[m];
			VuoPoint4d n = (VuoPoint4d){0,0,1,1};

			switch(element)
			{
				case 0:	// normals
					n = vertices.normals[m];
					break;
				case 1:	// tangents
					n = vertices.tangents[m];
					break;
				case 2:	// bitangents
					n = vertices.bitangents[m];
					break;
			}

			n = VuoPoint4d_add(v, VuoPoint4d_multiply(n, length));

			glVertex3d(v.x, v.y, v.z);
			glVertex3d(n.x, n.y, n.z);
		}
	}
	glEnd();

	glLoadIdentity();
}

/**
 * Draws a circle using OpenGL immediate mode.  For debugging only.
 */
static void drawCircle(CGLContextObj cgl_ctx, VuoPoint3d center, float radius, VuoPoint3d normal)
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
}

/**
 * Draws a cone using OpenGL immediate mode.  For debugging only.
 */
static void drawCone(CGLContextObj cgl_ctx, VuoPoint3d center, float radius, VuoPoint3d normal, float height)
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
}

/**
 * Draws the scene's point and spot lights.
 */
void VuoSceneRenderer_drawLights(VuoSceneRendererInternal *sceneRenderer)
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
		VuoSceneObject pointLight = VuoListGetValueAtIndex_VuoSceneObject(sceneRenderer->pointLights, i);
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
		VuoSceneObject spotLight = VuoListGetValueAtIndex_VuoSceneObject(sceneRenderer->spotLights, i);
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
}

/**
 * Uploads @c so to the GPU, and stores the uploaded object names in @c soi.
 * Does not traverse child objects.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_uploadSceneObject(VuoSceneObject so, VuoSceneRendererInternal_object *soi, VuoGlContext glContext)
{
	if (!so.verticesList || !so.shader)
		return;

	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	// Fetch the shader's attribute locations
	dispatch_semaphore_wait(so.shader->lock, DISPATCH_TIME_FOREVER);
	glUseProgram(so.shader->glProgramName);
	GLint positionAttribute = glGetAttribLocation(so.shader->glProgramName, "position");
	GLint normalAttribute = glGetAttribLocation(so.shader->glProgramName, "normal");
	GLint tangentAttribute = glGetAttribLocation(so.shader->glProgramName, "tangent");
	GLint bitangentAttribute = glGetAttribLocation(so.shader->glProgramName, "bitangent");
	GLint textureCoordinateAttribute = glGetAttribLocation(so.shader->glProgramName, "textureCoordinate");
	glUseProgram(0);
	dispatch_semaphore_signal(so.shader->lock);

	// For each VuoVertices in the sceneobject...
	unsigned long verticesListCount = VuoListGetCount_VuoVertices(so.verticesList);
	for (unsigned long i = 1; i <= verticesListCount; ++i)
	{
		VuoVertices vertices = VuoListGetValueAtIndex_VuoVertices(so.verticesList, i);
		VuoSceneRendererInternal_vertices v;


		// Combine the vertex attribute buffers together, so we only have to upload a single buffer:


		// Allocate client-side buffer
		unsigned int bufferCount = 0;
		++bufferCount; // positions
		if (vertices.normals && normalAttribute>=0)
			++bufferCount;
		if (vertices.tangents && tangentAttribute>=0)
			++bufferCount;
		if (vertices.bitangents && bitangentAttribute>=0)
			++bufferCount;
		if (vertices.textureCoordinates && textureCoordinateAttribute>=0)
			++bufferCount;
		unsigned long singleBufferSize = sizeof(VuoPoint4d)*vertices.vertexCount;
		VuoPoint4d *combinedData = (VuoPoint4d *)malloc(singleBufferSize*bufferCount);


		// Combine vertex attributes into the client-side buffer
		unsigned long combinedDataOffset = 0;
		memcpy(combinedData + singleBufferSize*(combinedDataOffset++), vertices.positions, singleBufferSize);

		void *normalOffset=0;
		if (vertices.normals && normalAttribute>=0)
		{
			normalOffset = (void *)(singleBufferSize*combinedDataOffset);
			memcpy((char *)combinedData + singleBufferSize*(combinedDataOffset++), vertices.normals, singleBufferSize);
		}

		void *tangentOffset=0;
		if (vertices.tangents && tangentAttribute>=0)
		{
			tangentOffset = (void *)(singleBufferSize*combinedDataOffset);
			memcpy((char *)combinedData + singleBufferSize*(combinedDataOffset++), vertices.tangents, singleBufferSize);
		}

		void *bitangentOffset=0;
		if (vertices.bitangents && bitangentAttribute>=0)
		{
			bitangentOffset = (void *)(singleBufferSize*combinedDataOffset);
			memcpy((char *)combinedData + singleBufferSize*(combinedDataOffset++), vertices.bitangents, singleBufferSize);
		}

		void *textureCoordinateOffset=0;
		if (vertices.textureCoordinates && textureCoordinateAttribute>=0)
		{
			textureCoordinateOffset = (void *)(singleBufferSize*combinedDataOffset);
			memcpy((char *)combinedData + singleBufferSize*(combinedDataOffset++), vertices.textureCoordinates, singleBufferSize);
		}


		// Create a Vertex Array Object, to store this sceneobject's vertex array bindings.
		glGenVertexArrays(1, &v.vertexArray);
		glBindVertexArray(v.vertexArray);


		// Upload the combined buffer.
		v.combinedBufferSize = singleBufferSize*bufferCount;
		v.combinedBuffer = VuoGlPool_use(glContext, VuoGlPool_ArrayBuffer, v.combinedBufferSize);
		glBindBuffer(GL_ARRAY_BUFFER, v.combinedBuffer);
		glBufferData(GL_ARRAY_BUFFER, singleBufferSize*bufferCount, combinedData, GL_STREAM_DRAW);
/// @todo https://b33p.net/kosada/node/6901
//		glBufferSubData(GL_ARRAY_BUFFER, 0, v.combinedBufferSize, combinedData);
		free(combinedData);


		// Populate the Vertes Array Object with the various vertex attributes.
		glEnableVertexAttribArray((GLuint)positionAttribute);
		glVertexAttribPointer((GLuint)positionAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d), (void*)0);

		if (vertices.normals && normalAttribute>=0)
		{
			glEnableVertexAttribArray((GLuint)normalAttribute);
			glVertexAttribPointer((GLuint)normalAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d), (void*)normalOffset);
		}

		if (vertices.tangents && tangentAttribute>=0)
		{
			glEnableVertexAttribArray((GLuint)tangentAttribute);
			glVertexAttribPointer((GLuint)tangentAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d), (void*)tangentOffset);
		}

		if (vertices.bitangents && bitangentAttribute>=0)
		{
			glEnableVertexAttribArray((GLuint)bitangentAttribute);
			glVertexAttribPointer((GLuint)bitangentAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d), (void*)bitangentOffset);
		}

		if (vertices.textureCoordinates && textureCoordinateAttribute>=0)
		{
			glEnableVertexAttribArray((GLuint)textureCoordinateAttribute);
			glVertexAttribPointer((GLuint)textureCoordinateAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d), (void*)textureCoordinateOffset);
		}


		// Upload the Element Buffer and add it to the Vertex Array Object
		v.elementBufferSize = sizeof(unsigned int)*vertices.elementCount;
		v.elementBuffer = VuoGlPool_use(glContext, VuoGlPool_ElementArrayBuffer, v.elementBufferSize);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, v.elementBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*vertices.elementCount, vertices.elements, GL_STREAM_DRAW);
/// @todo https://b33p.net/kosada/node/6901
//		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, v.elementBufferSize, vertices.elements);


		glBindVertexArray(0);

		soi->vertices.push_back(v);
	}
}

/**
 * Uploads @c so and its child objects to the GPU, and stores the uploaded object names in @c soi.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_uploadSceneObjectsRecursively(VuoSceneObject so, VuoSceneRendererInternal_object *soi, VuoGlContext glContext)
{
	VuoSceneRenderer_uploadSceneObject(so, soi, glContext);

	if (!so.childObjects)
		return;

	unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
	for (unsigned long i = 1; i <= childObjectCount; ++i)
	{
		VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
		VuoSceneRendererInternal_object childObjectInternal;

		VuoSceneRenderer_uploadSceneObjectsRecursively(childObject, &childObjectInternal, glContext);

		soi->childObjects.push_back(childObjectInternal);
	}
}

/**
 * Releases the GPU objects created by @c VuoSceneRenderer_uploadSceneObject.
 * Does not traverse child objects.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_cleanupSceneObject(VuoSceneObject so, VuoSceneRendererInternal_object *soi, VuoGlContext glContext)
{
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	for (std::list<VuoSceneRendererInternal_vertices>::iterator vi = soi->vertices.begin(); vi != soi->vertices.end(); ++vi)
	{
		VuoGlPool_disuse(glContext, VuoGlPool_ArrayBuffer, (*vi).combinedBufferSize, (*vi).combinedBuffer);
		(*vi).combinedBuffer = 0;

		VuoGlPool_disuse(glContext, VuoGlPool_ElementArrayBuffer, (*vi).elementBufferSize, (*vi).elementBuffer);
		(*vi).elementBuffer = 0;

		glDeleteVertexArrays(1, &(*vi).vertexArray);
		(*vi).vertexArray = 0;
	}
	soi->vertices.clear();
}

/**
 * Releases the GPU objects created by @c VuoSceneRenderer_uploadSceneObjectsRecursively.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_cleanupSceneObjectsRecursively(VuoSceneObject so, VuoSceneRendererInternal_object *soi, VuoGlContext glContext)
{
	unsigned int i = 1;
	for (std::list<VuoSceneRendererInternal_object>::iterator oi = soi->childObjects.begin(); oi != soi->childObjects.end(); ++oi, ++i)
	{
		VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
		VuoSceneRenderer_cleanupSceneObjectsRecursively(childObject, &(*oi), glContext);
	}

	VuoSceneRenderer_cleanupSceneObject(so, soi, glContext);

	soi->childObjects.clear();
}

/**
 * Releases the Vuo objects related to this scenegraph.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_releaseSceneObjectsRecursively(VuoSceneObject so)
{
	if (so.childObjects)
	{
		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
			VuoSceneRenderer_releaseSceneObjectsRecursively(childObject);
		}
	}

	VuoSceneObject_release(so);
}

/**
 * Deeply retains @c so.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAny
 */
void VuoSceneRenderer_retainSceneObjectsRecursively(VuoSceneObject so)
{
	VuoSceneObject_retain(so);

	if (!so.childObjects)
		return;

	unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
	for (unsigned long i = 1; i <= childObjectCount; ++i)
	{
		VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
		VuoSceneRenderer_retainSceneObjectsRecursively(childObject);
	}
}

/**
 * Changes the scenegraph to be rendered.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_setRootSceneObject(VuoSceneRenderer sr, VuoSceneObject rootSceneObject)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;

	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	VuoSceneRenderer_retainSceneObjectsRecursively(rootSceneObject);

	// Release the old scenegraph.
	if (sceneRenderer->scenegraphValid)
	{
		VuoSceneRenderer_cleanupSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, sceneRenderer->glContext);
		VuoSceneRenderer_releaseSceneObjectsRecursively(sceneRenderer->rootSceneObject);
		VuoRelease(sceneRenderer->pointLights);
		VuoRelease(sceneRenderer->spotLights);
	}

	// Upload the new scenegraph.
	sceneRenderer->rootSceneObject = rootSceneObject;
	VuoSceneRenderer_uploadSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, sceneRenderer->glContext);

	VuoSceneObject_findLights(sceneRenderer->rootSceneObject, &sceneRenderer->ambientColor, &sceneRenderer->ambientBrightness, &sceneRenderer->pointLights, &sceneRenderer->spotLights);
	VuoRetain(sceneRenderer->pointLights);
	VuoRetain(sceneRenderer->spotLights);

	sceneRenderer->scenegraphValid = true;
	sceneRenderer->needToRegenerateProjectionMatrix = true;
	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);
}

/**
 * Changes the OpenGL context on which the scenegraph can be rendered.
 *
 * Requires use of both the old and new OpenGL contexts.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_switchContext(VuoSceneRenderer sr, VuoGlContext newGlContext)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;

	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);

	// Clean up the scenegraph on the old context.
	if (sceneRenderer->scenegraphValid)
	{
		CGLContextObj cgl_ctx = (CGLContextObj)sceneRenderer->glContext;
		VuoSceneRenderer_cleanupSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, sceneRenderer->glContext);
		glFlushRenderAPPLE();
	}

	sceneRenderer->glContext = newGlContext;
	VuoSceneRenderer_prepareContext((CGLContextObj)newGlContext);

	// Upload the scenegraph on the new context.
	if (sceneRenderer->scenegraphValid)
		VuoSceneRenderer_uploadSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, sceneRenderer->glContext);

	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);
}

/**
 * Changes the name of the camera to look for.
 * The first camera whose name contains @c cameraName will be rendered (next time @c VuoSceneRenderer_draw() is called),
 * or, if no camera matches, @c VuoSceneObject_makeDefaultCamera() will be used.
 *
 * @threadAny
 */
void VuoSceneRenderer_setCameraName(VuoSceneRenderer sr, VuoText cameraName)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;

	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	{
		if (sceneRenderer->cameraName)
			VuoRelease(sceneRenderer->cameraName);

		sceneRenderer->cameraName = cameraName;
		VuoRetain(sceneRenderer->cameraName);

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

	if (sceneRenderer->scenegraphValid)
	{
		VuoSceneRenderer_cleanupSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, sceneRenderer->glContext);
		VuoSceneRenderer_releaseSceneObjectsRecursively(sceneRenderer->rootSceneObject);
	}

	if (sceneRenderer->cameraName)
		VuoRelease(sceneRenderer->cameraName);

	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);
	dispatch_release(sceneRenderer->scenegraphSemaphore);
	VuoSceneObject_release(sceneRenderer->rootSceneObject);

	free(sceneRenderer);
}

/**
 * Creates an OpenGL Framebuffer Object, and uses it to render the scene to @c image and @c depthImage.
 */
void VuoSceneRenderer_renderToImage(VuoSceneRenderer sr, VuoImage *image, VuoImage *depthImage)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;

	CGLContextObj cgl_ctx = (CGLContextObj)sceneRenderer->glContext;

	// Create a new GL Texture Object and Framebuffer Object.
	GLuint outputTexture = VuoGlTexturePool_use(sceneRenderer->glContext, GL_RGBA, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight, GL_RGBA);

	GLuint outputDepthTexture=0;
	if (depthImage)
		outputDepthTexture = VuoGlTexturePool_use(sceneRenderer->glContext, GL_DEPTH_COMPONENT, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight, GL_DEPTH_COMPONENT);

	GLuint outputFramebuffer;
	glGenFramebuffers(1, &outputFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, outputFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);
	if (depthImage)
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, outputDepthTexture, 0);

	{
		glViewport(0, 0, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight);
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		VuoSceneRenderer_draw(sceneRenderer);
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	if (depthImage)
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &outputFramebuffer);

	*image = VuoImage_make(outputTexture, GL_RGBA, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight);
	if (depthImage)
		*depthImage = VuoImage_make(outputDepthTexture, GL_DEPTH_COMPONENT, sceneRenderer->viewportWidth, sceneRenderer->viewportHeight);
}
