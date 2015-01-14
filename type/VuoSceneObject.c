/**
 * @file
 * VuoSceneObject implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoSceneObject.h"
#include "VuoList_VuoImage.h"
#include "VuoList_VuoVertices.h"
#include "VuoList_VuoSceneObject.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Scene Object",
					 "description" : "A 3D Object: visible (mesh), or virtual (group, light, camera).",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c",
						 "json"
					 ]
				 });
#endif
/// @}


/**
 * Creates a new, empty scene object.
 */
VuoSceneObject VuoSceneObject_makeEmpty(void)
{
	VuoSceneObject o;

	o.verticesList = NULL;
	o.shader = NULL;

	o.childObjects = NULL;

	o.cameraType = VuoSceneObject_NotACamera;

	o.name = NULL;
	o.transform = VuoTransform_valueFromJson(NULL);

	return o;
}

/**
 * Creates a visible (mesh) scene object.
 */
VuoSceneObject VuoSceneObject_make(VuoList_VuoVertices verticesList, VuoShader shader, VuoTransform transform, VuoList_VuoSceneObject childObjects)
{
	VuoSceneObject o;

	o.verticesList = verticesList;
	o.shader = shader;

	o.childObjects = childObjects;

	o.cameraType = VuoSceneObject_NotACamera;

	o.name = NULL;
	o.transform = transform;

	return o;
}

/**
 * Returns a scene object with the specified @c image.
 *
 * @threadAnyGL
 */
VuoSceneObject VuoSceneObject_makeImage(VuoImage image, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal alpha)
{
	if (!image)
		return VuoSceneObject_makeEmpty();

	VuoList_VuoVertices verticesList = VuoListCreate_VuoVertices();
	// Since we're speciying VuoShader_makeImageShader() which doesn't use normals, we don't need to generate them.
	VuoListAppendValue_VuoVertices(verticesList, VuoVertices_getQuadWithoutNormals());
	VuoSceneObject object = VuoSceneObject_make(
				verticesList,
				VuoShader_makeImageShader(),
				VuoTransform_makeEuler(
					center,
					VuoPoint3d_multiply(rotation, M_PI/180.),
					VuoPoint3d_make(width,image->pixelsHigh * width/image->pixelsWide,1)
				),
				NULL
			);

	{
		VuoGlContext glContext = VuoGlContext_use();

		VuoShader_addTexture(object.shader, glContext, "texture", image);

		VuoShader_setUniformFloat(object.shader, glContext, "alpha", alpha);

		VuoGlContext_disuse(glContext);
	}

	return object;
}

/**
 * Returns a scene object consisting of 6 child objects (square quads), each with its own shader.
 */
VuoSceneObject VuoSceneObject_makeCube(VuoTransform transform, VuoShader frontShader, VuoShader leftShader, VuoShader rightShader, VuoShader backShader, VuoShader topShader, VuoShader bottomShader)
{
	VuoList_VuoSceneObject cubeChildObjects = VuoListCreate_VuoSceneObject();

	VuoList_VuoVertices quadVertices = VuoListCreate_VuoVertices();
	VuoListAppendValue_VuoVertices(quadVertices, VuoVertices_getQuad());

	// Front Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadVertices,
					frontShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,0,.5), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Left Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadVertices,
					leftShader,
					VuoTransform_makeEuler(VuoPoint3d_make(-.5,0,0), VuoPoint3d_make(0,-M_PI/2.,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Right Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadVertices,
					rightShader,
					VuoTransform_makeEuler(VuoPoint3d_make(.5,0,0), VuoPoint3d_make(0,M_PI/2.,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Back Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadVertices,
					backShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,0,-.5), VuoPoint3d_make(0,M_PI,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Top Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadVertices,
					topShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,.5,0), VuoPoint3d_make(-M_PI/2.,0,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Bottom Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadVertices,
					bottomShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,-.5,0), VuoPoint3d_make(M_PI/2.,0,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	return VuoSceneObject_make(NULL, NULL, transform, cubeChildObjects);
}

/**
 * Returns a perspective camera.
 */
VuoSceneObject VuoSceneObject_makePerspectiveCamera(VuoText name, VuoPoint3d position, VuoPoint3d rotation, float fieldOfView, float distanceMin, float distanceMax)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.name = name;
	o.transform = VuoTransform_makeEuler(
				position,
				rotation,
				VuoPoint3d_make(1,1,1)
				);
	o.cameraType = VuoSceneObject_PerspectiveCamera;
	o.cameraFieldOfView = fieldOfView;
	o.cameraDistanceMin = distanceMin;
	o.cameraDistanceMax = distanceMax;
	return o;
}

/**
 * Returns an orthographic camera.
 */
VuoSceneObject VuoSceneObject_makeOrthographicCamera(VuoText name, VuoPoint3d position, VuoPoint3d rotation, float width, float distanceMin, float distanceMax)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.name = name;
	o.transform = VuoTransform_makeEuler(
				position,
				rotation,
				VuoPoint3d_make(1,1,1)
				);
	o.cameraType = VuoSceneObject_OrthographicCamera;
	o.cameraWidth = width;
	o.cameraDistanceMin = distanceMin;
	o.cameraDistanceMax = distanceMax;
	return o;
}

/**
 * Returns a perspective camera at (0,0,1), facing along -z, 90 degree FOV, and clip planes at 0.1 and 10.0.
 */
VuoSceneObject VuoSceneObject_makeDefaultCamera(void)
{
	return VuoSceneObject_makePerspectiveCamera(
				VuoText_make("default camera"),
				VuoPoint3d_make(0,0,1),
				VuoPoint3d_make(0,0,0),
				90,
				0.1,
				10.0
				);
}

/**
 * Performs a depth-first search of the scenegraph.
 * Returns the first camera whose name contains @c nameToMatch (or, if @c nameToMatch is emptystring, just returns the first camera).
 * Output paramater @c foundCamera indicates whether a camera was found.
 * If no camera was found, returns VuoSceneObject_makeDefaultCamera().
 *
 * @todo apply hierarchical transformations
 */
VuoSceneObject VuoSceneObject_findCamera(VuoSceneObject so, VuoText nameToMatch, bool *foundCamera)
{
	if (so.cameraType != VuoSceneObject_NotACamera && strstr(so.name,nameToMatch))
	{
		*foundCamera = true;
		return so;
	}

	if (so.childObjects)
	{
		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
			bool foundChildCamera;
			VuoSceneObject childCamera = VuoSceneObject_findCamera(childObject, nameToMatch, &foundChildCamera);
			if (foundChildCamera)
			{
				*foundCamera = true;
				return childCamera;
			}
		}
	}

	*foundCamera = false;
	return VuoSceneObject_makeDefaultCamera();
}

/**
 * @ingroup VuoSceneObject
 * Returns the @c VuoSceneObject_CameraType corresponding with the string @c cameraTypeString.  If none matches, returns VuoSceneObject_NotACamera.
 */
VuoSceneObject_CameraType VuoSceneObject_cameraTypeFromCString(const char *cameraTypeString)
{
	if (strcmp(cameraTypeString,"perspective")==0)
		return VuoSceneObject_PerspectiveCamera;
	else if (strcmp(cameraTypeString,"orthographic")==0)
		return VuoSceneObject_OrthographicCamera;

	return VuoSceneObject_NotACamera;
}

/**
 * @ingroup VuoSceneObject
 * Returns a string constant representing @c cameraType.
 */
const char * VuoSceneObject_cStringForCameraType(VuoSceneObject_CameraType cameraType)
{
	switch (cameraType)
	{
		case VuoSceneObject_PerspectiveCamera:
			return "perspective";
		case VuoSceneObject_OrthographicCamera:
			return "orthographic";
		default:
			return "notACamera";
	}
}

/**
 * @ingroup VuoSceneObject
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "verticesList" : ... ,
 *     "shader" : ... ,
 *     "childObjects" : ...,
 *     "transform" : ...
 *   }
 * }
 *
 * @eg{
 *   {
 *     "cameraType" : "perspective",
 *     "cameraFieldOfView" : 90.0,
 *     "cameraDistanceMin" : 0.1,
 *     "cameraDistanceMax" : 10.0,
 *     "name" : ...,
 *     "transform" : ...
 *   }
 * }
 */
VuoSceneObject VuoSceneObject_valueFromJson(json_object * js)
{
	json_object *o = NULL;

	VuoList_VuoVertices verticesList = NULL;
	if (json_object_object_get_ex(js, "verticesList", &o))
		verticesList = VuoList_VuoVertices_valueFromJson(o);

	VuoShader shader = NULL;
	if (json_object_object_get_ex(js, "shader", &o))
		shader = VuoShader_valueFromJson(o);

	VuoList_VuoSceneObject childObjects = NULL;
	if (json_object_object_get_ex(js, "childObjects", &o))
		childObjects = VuoList_VuoSceneObject_valueFromJson(o);

	VuoSceneObject_CameraType cameraType = VuoSceneObject_NotACamera;
	if (json_object_object_get_ex(js, "cameraType", &o))
		cameraType = VuoSceneObject_cameraTypeFromCString(json_object_get_string(o));

	float cameraFieldOfView;
	if (json_object_object_get_ex(js, "cameraFieldOfView", &o))
		cameraFieldOfView = json_object_get_double(o);

	float cameraWidth;
	if (json_object_object_get_ex(js, "cameraWidth", &o))
		cameraWidth = json_object_get_double(o);

	float cameraDistanceMin;
	if (json_object_object_get_ex(js, "cameraDistanceMin", &o))
		cameraDistanceMin = json_object_get_double(o);

	float cameraDistanceMax;
	if (json_object_object_get_ex(js, "cameraDistanceMax", &o))
		cameraDistanceMax = json_object_get_double(o);

	VuoText name = NULL;
	if (json_object_object_get_ex(js, "name", &o))
		name = VuoText_valueFromJson(o);

	json_object_object_get_ex(js, "transform", &o);
	VuoTransform transform = VuoTransform_valueFromJson(o);


	if (cameraType == VuoSceneObject_PerspectiveCamera)
		return VuoSceneObject_makePerspectiveCamera(
					name,
					transform.translation,
					transform.rotationSource.euler,
					cameraFieldOfView,
					cameraDistanceMin,
					cameraDistanceMax
					);
	else if (cameraType == VuoSceneObject_OrthographicCamera)
		return VuoSceneObject_makeOrthographicCamera(
					name,
					transform.translation,
					transform.rotationSource.euler,
					cameraWidth,
					cameraDistanceMin,
					cameraDistanceMax
					);
	else
		return VuoSceneObject_make(verticesList, shader, transform, childObjects);
}

/**
 * @ingroup VuoSceneObject
 * Encodes @c value as a JSON object.
 */
json_object * VuoSceneObject_jsonFromValue(const VuoSceneObject value)
{
	json_object *js = json_object_new_object();

	if (value.cameraType != VuoSceneObject_NotACamera)
	{
		json_object_object_add(js, "cameraType", json_object_new_string(VuoSceneObject_cStringForCameraType(value.cameraType)));
		json_object_object_add(js, "cameraDistanceMin", json_object_new_double(value.cameraDistanceMin));
		json_object_object_add(js, "cameraDistanceMax", json_object_new_double(value.cameraDistanceMax));
	}

	if (value.cameraType == VuoSceneObject_PerspectiveCamera)
		json_object_object_add(js, "cameraFieldOfView", json_object_new_double(value.cameraFieldOfView));
	else if (value.cameraType == VuoSceneObject_OrthographicCamera)
		json_object_object_add(js, "cameraWidth", json_object_new_double(value.cameraWidth));
	else // visible or group scene object
	{
		if (value.verticesList)
		{
			json_object *verticesListObject = VuoList_VuoVertices_jsonFromValue(value.verticesList);
			json_object_object_add(js, "verticesList", verticesListObject);
		}

		if (value.shader)
		{
			json_object *shaderObject = VuoShader_jsonFromValue(value.shader);
			json_object_object_add(js, "shader", shaderObject);
		}

		if (value.childObjects)
		{
			json_object *childObjectsObject = VuoList_VuoSceneObject_jsonFromValue(value.childObjects);
			json_object_object_add(js, "childObjects", childObjectsObject);
		}
	}

	if (value.name)
	{
		json_object *nameObject = VuoText_jsonFromValue(value.name);
		json_object_object_add(js, "name", nameObject);
	}

	json_object *transformObject = VuoTransform_jsonFromValue(value.transform);
	json_object_object_add(js, "transform", transformObject);

	return js;
}

/**
 * Returns the total number of vertices in the scene object (but not its descendants).
 */
unsigned long VuoSceneObject_getVertexCount(const VuoSceneObject value)
{
	if (!value.verticesList)
		return 0;

	unsigned long vertexCount = 0;
	unsigned long verticesListCount = VuoListGetCount_VuoVertices(value.verticesList);
	for (unsigned long i = 1; i <= verticesListCount; ++i)
		vertexCount += VuoListGetValueAtIndex_VuoVertices(value.verticesList, i).vertexCount;

	return vertexCount;
}

/**
 * Returns the total number of element in the scene object (but not its descendants).
 */
unsigned long VuoSceneObject_getElementCount(const VuoSceneObject value)
{
	if (!value.verticesList)
		return 0;

	unsigned long elementCount = 0;
	unsigned long verticesListCount = VuoListGetCount_VuoVertices(value.verticesList);
	for (unsigned long i = 1; i <= verticesListCount; ++i)
		elementCount += VuoListGetValueAtIndex_VuoVertices(value.verticesList, i).elementCount;

	return elementCount;
}

/**
 * Traverses the specified scenegraph and returns statistics about it.
 */
void VuoSceneObject_getStatistics(const VuoSceneObject value, unsigned long *descendantCount, unsigned long *totalVertexCount, unsigned long *totalElementCount, unsigned long *totalTextureCount)
{
	unsigned long childObjectCount = 0;
	if (value.childObjects)
		childObjectCount = VuoListGetCount_VuoSceneObject(value.childObjects);
	*descendantCount += childObjectCount;
	*totalVertexCount += VuoSceneObject_getVertexCount(value);
	*totalElementCount += VuoSceneObject_getElementCount(value);
	if (value.shader)
		*totalTextureCount += VuoListGetCount_VuoImage(value.shader->textures);

	for (unsigned long i = 1; i <= childObjectCount; ++i)
		VuoSceneObject_getStatistics(VuoListGetValueAtIndex_VuoSceneObject(value.childObjects, i), descendantCount, totalVertexCount, totalElementCount, totalTextureCount);
}

/**
 * @ingroup VuoSceneObject
 * Produces a brief human-readable summary of @c value.
 */
char * VuoSceneObject_summaryFromValue(const VuoSceneObject value)
{
	if (value.cameraType != VuoSceneObject_NotACamera)
	{
		const char *format = "%s camera<br>at (%s)<br>rotated (%s)<br>%g%s<br>shows objects between depth %g and %g";

		const char *cameraType = VuoSceneObject_cStringForCameraType(value.cameraType);

		float cameraViewValue = 0;
		const char *cameraViewString = "";
		if (value.cameraType == VuoSceneObject_PerspectiveCamera)
		{
			cameraViewValue = value.cameraFieldOfView;
			cameraViewString = "° field of view";
		}
		else if (value.cameraType == VuoSceneObject_OrthographicCamera)
		{
			cameraViewValue = value.cameraWidth;
			cameraViewString = " unit width";
		}

		const char *translationString = VuoPoint3d_summaryFromValue(value.transform.translation);
		const char *rotationString = VuoPoint3d_summaryFromValue(VuoPoint3d_multiply(value.transform.rotationSource.euler,180./M_PI));
		int size = snprintf(NULL, 0, format, cameraType, translationString, rotationString, cameraViewValue, cameraViewString, value.cameraDistanceMin, value.cameraDistanceMax);
		char *valueAsString = (char *)malloc(size+1);
		snprintf(valueAsString, size+1, format, cameraType, translationString, rotationString, cameraViewValue, cameraViewString, value.cameraDistanceMin, value.cameraDistanceMax);
		return valueAsString;
	}

	const char *format = "%d vertices, %d elements<br><br>%s<br><br>%s<br><br>%d child object%s%s";

	unsigned long vertexCount = VuoSceneObject_getVertexCount(value);
	unsigned long elementCount = VuoSceneObject_getElementCount(value);

	char *shader = "(no shader)";
	if (value.shader)
		shader = VuoShader_summaryFromValue(value.shader);
	char *transform = VuoTransform_summaryFromValue(value.transform);

	unsigned long childObjectCount = 0;
	if (value.childObjects)
		childObjectCount = VuoListGetCount_VuoSceneObject(value.childObjects);
	const char *childObjectPlural = childObjectCount == 1 ? "" : "s";

	char *descendants;
	if (childObjectCount)
	{
		unsigned long descendantCount = 0;
		unsigned long totalVertexCount = 0;
		unsigned long totalElementCount = 0;
		unsigned long totalTextureCount = 0;
		VuoSceneObject_getStatistics(value, &descendantCount, &totalVertexCount, &totalElementCount, &totalTextureCount);
		const char *descendantPlural = descendantCount == 1 ? "" : "s";
		const char *totalTexturePlural = totalTextureCount == 1 ? "" : "s";

		const char *descendantsFormat = "<br>%d descendant%s<br><br>total, including descendants:<br>%d vertices, %d elements<br>%d texture%s";
		int size = snprintf(NULL, 0, descendantsFormat, descendantCount, descendantPlural, totalVertexCount, totalElementCount, totalTextureCount, totalTexturePlural);
		descendants = (char *)malloc(size+1);
		snprintf(descendants, size+1, descendantsFormat, descendantCount, descendantPlural, totalVertexCount, totalElementCount, totalTextureCount, totalTexturePlural);
	}
	else
		descendants = strdup("");

	int size = snprintf(NULL, 0, format, vertexCount, elementCount, shader, transform, childObjectCount, childObjectPlural, descendants);
	char *valueAsString = (char *)malloc(size+1);
	snprintf(valueAsString, size+1, format, vertexCount, elementCount, shader, transform, childObjectCount, childObjectPlural, descendants);

	free(descendants);
	free(transform);
	if (value.shader)
		free(shader);

	return valueAsString;
}

static void VuoSceneObject_dump_internal(const VuoSceneObject so, unsigned int level)
{
	for (unsigned int i=0; i<level; ++i)
		fprintf(stderr, "\t");

	fprintf(stderr, "object: %lu vertices, %lu elements\n", VuoSceneObject_getVertexCount(so), VuoSceneObject_getElementCount(so));

	if (so.childObjects)
	{
		unsigned int childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
		for (unsigned int i=1; i<=childObjectCount; ++i)
			VuoSceneObject_dump_internal(VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i), level+1);
	}
}

/**
 * Outputs information about the sceneobject (and its descendants).
 */
void VuoSceneObject_dump(const VuoSceneObject so)
{
	VuoSceneObject_dump_internal(so,0);
}
