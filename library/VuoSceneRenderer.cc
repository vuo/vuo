/**
 * @file
 * VuoSceneRenderer implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
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
	GLuint positionBuffer;
	GLuint normalBuffer;
	GLuint tangentBuffer;
	GLuint bitangentBuffer;
	GLuint textureCoordinateBuffer;
	GLuint elementBuffer;
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
};

void VuoSceneRenderer_destroy(VuoSceneRenderer sr);

/**
 * Creates a reference-counted object for rendering a scenegraph.
 *
 * @threadAny
 */
VuoSceneRenderer VuoSceneRenderer_make(void)
{
	VuoSceneRendererInternal *sceneRenderer = new VuoSceneRendererInternal;
	VuoRegister(sceneRenderer, VuoSceneRenderer_destroy);

	sceneRenderer->scenegraphSemaphore = dispatch_semaphore_create(1);
	sceneRenderer->scenegraphValid = false;
	sceneRenderer->needToRegenerateProjectionMatrix = false;
	sceneRenderer->cameraName = NULL;

	return (VuoSceneRenderer)sceneRenderer;
}

/**
 * Sets up OpenGL state on the current GL Context.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_prepareContext(VuoSceneRenderer sceneRenderer, VuoGlContext glContext)
{
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	glEnable(GL_MULTISAMPLE);

	glEnable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
				// Search again, and this time just pick the first camera we find.
				camera = VuoSceneObject_findCamera(sceneRenderer->rootSceneObject, "", &foundCamera);
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

	float rotationMatrix[4][4];
	VuoTransform_getMatrix(VuoTransform_makeEuler(
							   VuoPoint3d_make(0,0,0),
							   VuoPoint3d_multiply(camera.transform.rotationSource.euler,-1),
							   VuoPoint3d_make(1,1,1)),
						   (float *)rotationMatrix);

	float outputMatrix[4][4];
	VuoTransform_multiplyMatrices4x4((float *)rotationMatrix, sceneRenderer->projectionMatrix, (float *)&outputMatrix);

	float translationMatrix[4][4];
	VuoTransform_getMatrix(VuoTransform_makeEuler(
							   VuoPoint3d_multiply(camera.transform.translation,-1),
							   VuoPoint3d_make(0,0,0),
							   VuoPoint3d_make(1,1,1)),
						   (float *)translationMatrix);

	float outputMatrix2[4][4];
	VuoTransform_multiplyMatrices4x4((float *)translationMatrix, (float *)outputMatrix, (float *)&outputMatrix2);

	VuoTransform_copyMatrix4x4((float *)outputMatrix2, sceneRenderer->projectionMatrix);
}

/**
 * Draws @c so (using the uploaded object names in @c soi).
 * Does not traverse child objects.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_drawSceneObject(VuoSceneObject so, VuoSceneRendererInternal_object *soi, float projectionMatrix[16], float modelviewMatrix[16], VuoGlContext glContext)
{
	if (!so.shader)
		return;

	dispatch_semaphore_wait(so.shader->lock, DISPATCH_TIME_FOREVER);
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;
	glUseProgram(so.shader->glProgramName);
	{
		GLint projectionMatrixUniform = glGetUniformLocation(so.shader->glProgramName, "projectionMatrix");
		glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, projectionMatrix);

		GLint modelviewMatrixUniform = glGetUniformLocation(so.shader->glProgramName, "modelviewMatrix");
		glUniformMatrix4fv(modelviewMatrixUniform, 1, GL_FALSE, modelviewMatrix);

		VuoShader_activateTextures(so.shader, cgl_ctx);

		unsigned int i = 1;
		for (std::list<VuoSceneRendererInternal_vertices>::iterator vi = soi->vertices.begin(); vi != soi->vertices.end(); ++vi, ++i)
		{
			GLint positionAttribute = glGetAttribLocation(so.shader->glProgramName, "position");
			glBindBuffer(GL_ARRAY_BUFFER, (*vi).positionBuffer);
			glVertexAttribPointer((GLuint)positionAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d), (void*)0);
			glEnableVertexAttribArray((GLuint)positionAttribute);

			GLint normalAttribute = glGetAttribLocation(so.shader->glProgramName, "normal");
			if (normalAttribute >= 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, (*vi).normalBuffer);
				glVertexAttribPointer((GLuint)normalAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d), (void*)0);
				glEnableVertexAttribArray((GLuint)normalAttribute);
			}

			GLint tangentAttribute = glGetAttribLocation(so.shader->glProgramName, "tangent");
			if (tangentAttribute >= 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, (*vi).tangentBuffer);
				glVertexAttribPointer((GLuint)tangentAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d), (void*)0);
				glEnableVertexAttribArray((GLuint)tangentAttribute);
			}

			GLint bitangentAttribute = glGetAttribLocation(so.shader->glProgramName, "bitangent");
			if (bitangentAttribute >= 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, (*vi).bitangentBuffer);
				glVertexAttribPointer((GLuint)bitangentAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d), (void*)0);
				glEnableVertexAttribArray((GLuint)bitangentAttribute);
			}

			GLint textureCoordinateAttribute = glGetAttribLocation(so.shader->glProgramName, "textureCoordinate");
			if (textureCoordinateAttribute >= 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, (*vi).textureCoordinateBuffer);
				glVertexAttribPointer((GLuint)textureCoordinateAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d), (void*)0);
				glEnableVertexAttribArray((GLuint)textureCoordinateAttribute);
			}

			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (*vi).elementBuffer);
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
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}

			if (textureCoordinateAttribute >= 0)
				glDisableVertexAttribArray((GLuint)textureCoordinateAttribute);
			if (bitangentAttribute >= 0)
				glDisableVertexAttribArray((GLuint)bitangentAttribute);
			if (tangentAttribute >= 0)
				glDisableVertexAttribArray((GLuint)tangentAttribute);
			if (normalAttribute >= 0)
				glDisableVertexAttribArray((GLuint)normalAttribute);
			glDisableVertexAttribArray((GLuint)positionAttribute);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		VuoShader_deactivateTextures(so.shader, cgl_ctx);
	}
	glUseProgram(0);
	dispatch_semaphore_signal(so.shader->lock);
}

/**
 * Draws @c so and its child objects.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_drawSceneObjectsRecursively(VuoSceneObject so, VuoSceneRendererInternal_object *soi, float projectionMatrix[16], float modelviewMatrix[16], VuoGlContext glContext)
{
	float localModelviewMatrix[16];
	VuoTransform_getMatrix(so.transform, localModelviewMatrix);
	float compositeModelviewMatrix[16];
	VuoTransform_multiplyMatrices4x4(localModelviewMatrix, modelviewMatrix, compositeModelviewMatrix);
	VuoSceneRenderer_drawSceneObject(so, soi, projectionMatrix, compositeModelviewMatrix, glContext);

	unsigned int i = 1;
	for (std::list<VuoSceneRendererInternal_object>::iterator oi = soi->childObjects.begin(); oi != soi->childObjects.end(); ++oi, ++i)
	{
		VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
		VuoSceneRenderer_drawSceneObjectsRecursively(childObject, &(*oi), projectionMatrix, compositeModelviewMatrix, glContext);
	}
}

/**
 * Renders the scene.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_draw(VuoSceneRenderer sr, VuoGlContext glContext)
{
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;

	if (!sceneRenderer->scenegraphValid)
		return;

	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);

	if (sceneRenderer->needToRegenerateProjectionMatrix)
	{
		VuoSceneRenderer_regenerateProjectionMatrixInternal(sceneRenderer);
		sceneRenderer->needToRegenerateProjectionMatrix = false;
	}

	// Allocate a single vertex array for all drawing during this pass (since VAOs can't be shared between contexts).
	GLuint vertexArray;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	float localModelviewMatrix[16];
	VuoTransform_getMatrix(VuoTransform_makeIdentity(), localModelviewMatrix);
	VuoSceneRenderer_drawSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, sceneRenderer->projectionMatrix, localModelviewMatrix, glContext);

	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);

	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vertexArray);
}

/**
 * Draws all vertex normals in sceneRenderer-rootSceneObject.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_drawElement(VuoSceneRenderer sr, VuoGlContext glContext, int element, float length)	// TODO Enum type for element to debug
{
	/// @todo update VuoSceneRenderer_draw() to call this for each VuoSceneObject when debugging
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;

	if (!sceneRenderer->scenegraphValid)
		return;

	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	unsigned long verticesListCount = VuoListGetCount_VuoVertices(sceneRenderer->rootSceneObject.verticesList);

	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(sceneRenderer->projectionMatrix);


	glMatrixMode(GL_MODELVIEW);

	float modelViewMatrix[16];
	VuoTransform_getMatrix(sceneRenderer->rootSceneObject.transform, modelViewMatrix);
	glLoadMatrixf(modelViewMatrix);

	glBegin(GL_LINES);
	for (unsigned int i = 1; i <= verticesListCount; i++)
	{
		VuoVertices vertices = VuoListGetValueAtIndex_VuoVertices(sceneRenderer->rootSceneObject.verticesList, i);

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

			n = VuoPoint4d_subtract(v, VuoPoint4d_multiply(n, length));

			glVertex3d(v.x, v.y, v.z);
			glVertex3d(n.x, n.y, n.z);
		}
	}
	glEnd();

	glLoadIdentity();

	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);
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
	if (!so.verticesList)
		return;

	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	unsigned long verticesListCount = VuoListGetCount_VuoVertices(so.verticesList);
	for (unsigned long i = 1; i <= verticesListCount; ++i)
	{
		VuoVertices vertices = VuoListGetValueAtIndex_VuoVertices(so.verticesList, i);
		VuoSceneRendererInternal_vertices v;

		v.positionBuffer = VuoGlPool_use(VuoGlPool_ArrayBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, v.positionBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VuoPoint4d)*vertices.vertexCount, vertices.positions, GL_STREAM_DRAW);

		v.normalBuffer = VuoGlPool_use(VuoGlPool_ArrayBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, v.normalBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VuoPoint4d)*vertices.vertexCount, vertices.normals, GL_STREAM_DRAW);

		v.tangentBuffer = VuoGlPool_use(VuoGlPool_ArrayBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, v.tangentBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VuoPoint4d)*vertices.vertexCount, vertices.tangents, GL_STREAM_DRAW);

		v.bitangentBuffer = VuoGlPool_use(VuoGlPool_ArrayBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, v.bitangentBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VuoPoint4d)*vertices.vertexCount, vertices.bitangents, GL_STREAM_DRAW);

		v.textureCoordinateBuffer = VuoGlPool_use(VuoGlPool_ArrayBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, v.textureCoordinateBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VuoPoint4d)*vertices.vertexCount, vertices.textureCoordinates, GL_STREAM_DRAW);

		{
			GLuint vertexArray;
			glGenVertexArrays(1, &vertexArray);
			glBindVertexArray(vertexArray);

			v.elementBuffer = VuoGlPool_use(VuoGlPool_ElementArrayBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, v.elementBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*vertices.elementCount, vertices.elements, GL_STREAM_DRAW);

			glBindVertexArray(0);
			glDeleteVertexArrays(1, &vertexArray);
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

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
void VuoSceneRenderer_releaseSceneObject(VuoSceneObject so, VuoSceneRendererInternal_object *soi, VuoGlContext glContext)
{
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	for (std::list<VuoSceneRendererInternal_vertices>::iterator vi = soi->vertices.begin(); vi != soi->vertices.end(); ++vi)
	{
		VuoGlPool_disuse(VuoGlPool_ArrayBuffer, (*vi).positionBuffer);
		VuoGlPool_disuse(VuoGlPool_ArrayBuffer, (*vi).normalBuffer);
		VuoGlPool_disuse(VuoGlPool_ArrayBuffer, (*vi).tangentBuffer);
		VuoGlPool_disuse(VuoGlPool_ArrayBuffer, (*vi).bitangentBuffer);
		VuoGlPool_disuse(VuoGlPool_ArrayBuffer, (*vi).textureCoordinateBuffer);

		/// @todo https://b33p.net/kosada/node/6752 — Why does this leak if we recycle it?
		glDeleteBuffers(1,&(*vi).elementBuffer);
//		VuoGlPool_disuse(VuoGlPool_ElementArrayBuffer, (*vi).elementBuffer);
	}
	soi->vertices.clear();

	VuoSceneObject_release(so);
}

/**
 * Releases the GPU objects created by @c VuoSceneRenderer_uploadSceneObjectsRecursively.
 *
 * Must be called while scenegraphSemaphore is locked.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_releaseSceneObjectsRecursively(VuoSceneObject so, VuoSceneRendererInternal_object *soi, VuoGlContext glContext)
{
	unsigned int i = 1;
	for (std::list<VuoSceneRendererInternal_object>::iterator oi = soi->childObjects.begin(); oi != soi->childObjects.end(); ++oi, ++i)
	{
		VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
		VuoSceneRenderer_releaseSceneObjectsRecursively(childObject, &(*oi), glContext);
	}

	soi->childObjects.clear();

	VuoSceneRenderer_releaseSceneObject(so, soi, glContext);
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
void VuoSceneRenderer_setRootSceneObject(VuoSceneRenderer sr, VuoGlContext glContext, VuoSceneObject rootSceneObject)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;

	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	VuoSceneRenderer_retainSceneObjectsRecursively(rootSceneObject);

	// Release the old scenegraph.
	if (sceneRenderer->scenegraphValid)
		VuoSceneRenderer_releaseSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, glContext);

	// Upload the new scenegraph.
	sceneRenderer->rootSceneObject = rootSceneObject;
	VuoSceneRenderer_uploadSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, glContext);

	sceneRenderer->scenegraphValid = true;
	sceneRenderer->needToRegenerateProjectionMatrix = true;
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
		VuoGlContext glContext = VuoGlContext_use();

		VuoSceneRenderer_releaseSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, glContext);

		VuoGlContext_disuse(glContext);
	}

	if (sceneRenderer->cameraName)
		VuoRelease(sceneRenderer->cameraName);

	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);
	dispatch_release(sceneRenderer->scenegraphSemaphore);
	VuoSceneObject_release(sceneRenderer->rootSceneObject);

	free(sceneRenderer);
}
