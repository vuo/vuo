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
					 "description" : "A renderable 3D Object.",
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
	o.transform = VuoTransform_valueFromJson(NULL);
	o.childObjects = NULL;
	return o;
}

/**
 * Creates a scene object.
 */
VuoSceneObject VuoSceneObject_make(VuoList_VuoVertices verticesList, VuoShader shader, VuoTransform transform, VuoList_VuoSceneObject childObjects)
{
	VuoSceneObject o;
	o.verticesList = verticesList;
	o.shader = shader;
	o.transform = transform;
	o.childObjects = childObjects;
	return o;
}

/**
 * @ingroup VuoSceneObject
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "verticesList" : ... ,
 *     "shader" : ... ,
 *     "transform" : ... ,
 *     "childObjects" : ...
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

	json_object_object_get_ex(js, "transform", &o);
	VuoTransform transform = VuoTransform_valueFromJson(o);

	VuoList_VuoSceneObject childObjects = NULL;
	if (json_object_object_get_ex(js, "childObjects", &o))
		childObjects = VuoList_VuoSceneObject_valueFromJson(o);

	return VuoSceneObject_make(verticesList, shader, transform, childObjects);
}

/**
 * @ingroup VuoSceneObject
 * Encodes @c value as a JSON object.
 */
json_object * VuoSceneObject_jsonFromValue(const VuoSceneObject value)
{
	json_object *js = json_object_new_object();

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

	json_object *transformObject = VuoTransform_jsonFromValue(value.transform);
	json_object_object_add(js, "transform", transformObject);

	if (value.childObjects)
	{
		json_object *childObjectsObject = VuoList_VuoSceneObject_jsonFromValue(value.childObjects);
		json_object_object_add(js, "childObjects", childObjectsObject);
	}

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
