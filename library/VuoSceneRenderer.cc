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

	VuoGlContext glContext;
};

void VuoSceneRenderer_destroy(VuoSceneRenderer sr);

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

	return (VuoSceneRenderer)sceneRenderer;
}

/**
 * Sets up OpenGL state on the current GL Context.
 *
 * @threadAnyGL
 */
void VuoSceneRenderer_prepareContext(VuoSceneRenderer sr)
{
	VuoSceneRendererInternal *sceneRenderer = (VuoSceneRendererInternal *)sr;

	CGLContextObj cgl_ctx = (CGLContextObj)sceneRenderer->glContext;

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
			VuoTransform_getMatrix(VuoTransform_makeIdentity(), billboardMatrix);

			// Apply scale to make the image appear at real size (1:1).
			billboardMatrix[0] = 2. * image->pixelsWide/sceneRenderer->viewportWidth;
			billboardMatrix[5] = billboardMatrix[0] * image->pixelsHigh/image->pixelsWide;

			// Apply existing 2D translation.
				// Align the translation to pixel boundaries
				billboardMatrix[12] = floor((modelviewMatrix[12]+1.)/2.*sceneRenderer->viewportWidth) / ((float)sceneRenderer->viewportWidth) * 2. - 1.;
				billboardMatrix[13] = floor((modelviewMatrix[13]+1.)/2.*sceneRenderer->viewportWidth) / ((float)sceneRenderer->viewportWidth) * 2. - 1.;

				// Account for odd-dimensioned image
				billboardMatrix[12] += (image->pixelsWide % 2 ? (1./sceneRenderer->viewportWidth) : 0);
				billboardMatrix[13] -= (image->pixelsHigh % 2 ? (1./sceneRenderer->viewportWidth) : 0);

				// Account for odd-dimensioned viewport
				billboardMatrix[13] += (sceneRenderer->viewportWidth  % 2 ? (1./sceneRenderer->viewportWidth) : 0);
				billboardMatrix[13] -= (sceneRenderer->viewportHeight % 2 ? (1./sceneRenderer->viewportWidth) : 0);

			glUniformMatrix4fv(modelviewMatrixUniform, 1, GL_FALSE, billboardMatrix);
		}
		else
			glUniformMatrix4fv(modelviewMatrixUniform, 1, GL_FALSE, modelviewMatrix);

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
//	VuoSceneRenderer_drawElement(so, projectionMatrix, compositeModelviewMatrix, glContext, 0, .08f);	// Normals
//	VuoSceneRenderer_drawElement(so, projectionMatrix, compositeModelviewMatrix, glContext, 1, .08f);	// Tangents
//	VuoSceneRenderer_drawElement(so, projectionMatrix, compositeModelviewMatrix, glContext, 2, .08f);	// Bitangents

	unsigned int i = 1;
	for (std::list<VuoSceneRendererInternal_object>::iterator oi = soi->childObjects.begin(); oi != soi->childObjects.end(); ++oi, ++i)
	{
		VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
		VuoSceneRenderer_drawSceneObjectsRecursively(childObject, &(*oi), projectionMatrix, compositeModelviewMatrix, sceneRenderer);
	}
}

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
	}

	// Upload the new scenegraph.
	sceneRenderer->rootSceneObject = rootSceneObject;
	VuoSceneRenderer_uploadSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, sceneRenderer->glContext);

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

	// Upload the scenegraph on the new context.
	sceneRenderer->glContext = newGlContext;
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
