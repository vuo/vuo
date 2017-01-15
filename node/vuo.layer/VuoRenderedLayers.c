/**
 * @file
 * VuoRenderedLayers implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <string.h>
#include "type.h"
#include "VuoRenderedLayers.h"
#include "VuoList_VuoSceneObject.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Rendered Layers",
					 "description" : "A set of layers, transformed to their final positions for rendering",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoLayer",
						 "VuoSceneObject",
						 "VuoWindowReference",
						 "VuoList_VuoSceneObject"
					 ]
				 });
#endif
/// @}


/**
 * Creates a VuoRenderedLayers with the given rendering root layer, rendering dimensions, and no window.
 */
VuoRenderedLayers VuoRenderedLayers_make(VuoSceneObject rootSceneObject, unsigned long int pixelsWide, unsigned long int pixelsHigh, float backingScaleFactor)
{
	VuoRenderedLayers rl;
	rl.rootSceneObject = rootSceneObject;
	rl.pixelsWide = pixelsWide;
	rl.pixelsHigh = pixelsHigh;
	rl.backingScaleFactor = backingScaleFactor;
	rl.window = 0;
	return rl;
}

/**
 * Creates a VuoRenderedLayers with the given rendering root layer, rendering dimensions, and window.
 */
VuoRenderedLayers VuoRenderedLayers_makeWithWindow(VuoSceneObject rootSceneObject,
												   unsigned long int pixelsWide, unsigned long int pixelsHigh,
												   float backingScaleFactor,
												   VuoWindowReference window)
{
	VuoRenderedLayers rl;
	rl.rootSceneObject = rootSceneObject;
	rl.pixelsWide = pixelsWide;
	rl.pixelsHigh = pixelsHigh;
	rl.backingScaleFactor = backingScaleFactor;
	rl.window = window;
	return rl;
}

/**
 * Creates a VuoRenderedLayers with no root layer and zero size.
 */
VuoRenderedLayers VuoRenderedLayers_makeEmpty(void)
{
	return VuoRenderedLayers_make(VuoSceneObject_makeEmpty(), 0, 0, 1);
}

/**
 * Finds the layer with the given name, along with its ancestor layers.
 */
bool VuoRenderedLayers_findLayer(VuoRenderedLayers renderedLayers, VuoText layerName, VuoList_VuoSceneObject ancestorObjects, VuoSceneObject *foundObject)
{
	bool found = VuoSceneObject_find(renderedLayers.rootSceneObject, layerName, ancestorObjects, foundObject);
	return (found &&
			foundObject->mesh &&
			foundObject->mesh->submeshCount == 1 &&
			foundObject->mesh->submeshes[0].vertexCount == 4);
}

/**
 * Outputs the center and corner points of the layer with the given name, as transformed in @a renderedLayers.
 *
 * Return true if the layer is found in @a renderedLayers.
 */
void VuoRenderedLayers_getTransformedLayer(VuoRenderedLayers renderedLayers, VuoList_VuoSceneObject ancestorObjects, VuoSceneObject targetObject, VuoPoint2d *layerCenter, VuoPoint2d layerCorners[4])
{
	// Get the layer's corner points.
	VuoSubmesh layerQuad = targetObject.mesh->submeshes[0];
	VuoPoint3d layerCorners3d[] = { VuoPoint3d_make(layerQuad.positions[0].x, layerQuad.positions[0].y, 0),
									VuoPoint3d_make(layerQuad.positions[1].x, layerQuad.positions[1].y, 0),
									VuoPoint3d_make(layerQuad.positions[2].x, layerQuad.positions[2].y, 0),
									VuoPoint3d_make(layerQuad.positions[3].x, layerQuad.positions[3].y, 0) };

	for (int i = 0; i < 4; ++i)
	{
		layerCorners3d[i].x *= targetObject.shader->objectScale;
		layerCorners3d[i].y *= targetObject.shader->objectScale;
	}

	// Transform the layer to the rendered layers' coordinate space.
	VuoListAppendValue_VuoSceneObject(ancestorObjects, targetObject);
	VuoPoint3d layerCenter3d = VuoPoint3d_make(0,0,0);
	float matrix[16];
	if (targetObject.isRealSize)
	{
		// Real-size layer:
		// Apply the layer's transformations to the center point.
		unsigned long ancestorObjectCount = VuoListGetCount_VuoSceneObject(ancestorObjects);
		for (unsigned long i = ancestorObjectCount; i >= 1; --i)
		{
			VuoSceneObject ancestorObject = VuoListGetValue_VuoSceneObject(ancestorObjects, i);
			VuoTransform_getMatrix(ancestorObject.transform, matrix);
			layerCenter3d = VuoTransform_transformPoint(matrix, layerCenter3d);
		}

		// Scale the layer's corner points to the rendered layers' coordinate space.
		VuoImage image = VuoShader_getUniform_VuoImage(targetObject.shader, "texture");
		float widthScale = 2. * (float)image->pixelsWide / (float)renderedLayers.pixelsWide;
		float heightScale = widthScale * (float)image->pixelsHigh / (float)image->pixelsWide;
		for (int i = 0; i < 4; ++i)
		{
			layerCorners3d[i].x *= widthScale;
			layerCorners3d[i].y *= heightScale;
		}

		// Move the layer's corner points to match the center point.
		for (int i = 0; i < 4; ++i)
		{
			layerCorners3d[i].x += layerCenter3d.x;
			layerCorners3d[i].y += layerCenter3d.y;
		}
	}
	else
	{
		// Scaled layer:
		// Apply the layer's transformations to each of its corner points.
		unsigned long ancestorObjectCount = VuoListGetCount_VuoSceneObject(ancestorObjects);
		for (unsigned long i = ancestorObjectCount; i >= 1; --i)
		{
			VuoSceneObject ancestorObject = VuoListGetValue_VuoSceneObject(ancestorObjects, i);
			VuoTransform_getMatrix(ancestorObject.transform, matrix);
			for (int i = 0; i < 4; ++i)
				layerCorners3d[i] = VuoTransform_transformPoint(matrix, layerCorners3d[i]);
			layerCenter3d = VuoTransform_transformPoint(matrix, layerCenter3d);
		}
	}

	for (int i = 0; i < 4; ++i)
		layerCorners[i] = VuoPoint2d_make(layerCorners3d[i].x, layerCorners3d[i].y);
	*layerCenter = VuoPoint2d_make(layerCenter3d.x, layerCenter3d.y);
}

/**
 * Returns the axis-aligned bounding box for the 4 points.
 */
VuoRectangle VuoRenderedLayers_getBoundingBox(VuoPoint2d layerCorners[4])
{
	VuoPoint2d min = VuoPoint2d_make( INFINITY,  INFINITY);
	VuoPoint2d max = VuoPoint2d_make(-INFINITY, -INFINITY);
	for (int i = 0; i < 4; ++i)
	{
		if (layerCorners[i].x < min.x)
			min.x = layerCorners[i].x;
		if (layerCorners[i].x > max.x)
			max.x = layerCorners[i].x;
		if (layerCorners[i].y < min.y)
			min.y = layerCorners[i].y;
		if (layerCorners[i].y > max.y)
			max.y = layerCorners[i].y;
	}
	return VuoRectangle_make(
				min.x + (max.x-min.x)/2.,
				min.y + (max.y-min.y)/2.,
				max.x-min.x,
				max.y-min.y);
}

/**
 * Returns true if the given point is within (or on the boundary) of the quad with the given corner points.
 */
bool VuoRenderedLayers_isPointInQuad(VuoPoint2d corners[4], VuoPoint2d point)
{
	// Split the quad into two triangles with points specified counter-clockwise, and check each triangle.
	// http://stackoverflow.com/questions/2049582/how-to-determine-a-point-in-a-triangle
	VuoPoint2d triangles[] = { corners[0], corners[1], corners[2],
									corners[3], corners[2], corners[1] };
	for (int i = 0; i < 2; ++i)
	{
		VuoPoint2d p0 = triangles[3*i];
		VuoPoint2d p1 = triangles[3*i+1];
		VuoPoint2d p2 = triangles[3*i+2];
		float area = 1./2.*(-p1.y*p2.x + p0.y*(-p1.x + p2.x) + p0.x*(p1.y - p2.y) + p1.x*p2.y);
		float s = 1./(2.*area)*(p0.y*p2.x - p0.x*p2.y + (p2.y - p0.y)*point.x + (p0.x - p2.x)*point.y);
		float t = 1./(2.*area)*(p0.x*p1.y - p0.y*p1.x + (p0.y - p1.y)*point.x + (p1.x - p0.x)*point.y);
		if (s >= 0 && t >= 0 && s + t <= 1)
			return true;
	}

	return false;
}

/**
 * Returns true if the given point is within (or on the boundary) of the layer with the given name.
 *
 * @a point should be in the same coordinate space as @a renderedLayers.
 */
bool VuoRenderedLayers_isPointInLayer(VuoRenderedLayers renderedLayers, VuoText layerName, VuoPoint2d point)
{
	// Check if the point is within the rendered layers.
	double yMaxInVuoCoordinates = (double)renderedLayers.pixelsHigh / (double)renderedLayers.pixelsWide;
	if (! (-1 <= point.x && point.x <= 1 &&
		   -yMaxInVuoCoordinates <= point.y && point.y <= yMaxInVuoCoordinates) )
		return false;

	// Check if the point is within the layer's transformed corner points.
	bool isPointInLayer = false;
	VuoSceneObject targetObject;
	VuoList_VuoSceneObject ancestorObjects = VuoListCreate_VuoSceneObject();
	bool isLayerFound = VuoRenderedLayers_findLayer(renderedLayers, layerName, ancestorObjects, &targetObject);
	if (isLayerFound)
	{
		VuoPoint2d layerCenter;
		VuoPoint2d layerCorners[4];
		VuoRenderedLayers_getTransformedLayer(renderedLayers, ancestorObjects, targetObject, &layerCenter, layerCorners);
		isPointInLayer = VuoRenderedLayers_isPointInQuad(layerCorners, point);
	}

	VuoRetain(ancestorObjects);
	VuoRelease(ancestorObjects);
	return isPointInLayer;
}

/**
 * @ingroup VuoRenderedLayers
 * @see VuoSceneObject_makeFromJson
 */
VuoRenderedLayers VuoRenderedLayers_makeFromJson(json_object * js)
{
	json_object *o = NULL;

	VuoSceneObject rootSceneObject;
	if (json_object_object_get_ex(js, "rootSceneObject", &o))
		rootSceneObject = VuoSceneObject_makeFromJson(o);
	else
		rootSceneObject = VuoSceneObject_makeEmpty();

	VuoInteger pixelsWide = 0;
	if (json_object_object_get_ex(js, "pixelsWide", &o))
		pixelsWide = json_object_get_int64(o);

	VuoInteger pixelsHigh = 0;
	if (json_object_object_get_ex(js, "pixelsHigh", &o))
		pixelsHigh = json_object_get_int64(o);

	VuoWindowReference window = 0;
	if (json_object_object_get_ex(js, "window", &o))
		window = VuoWindowReference_makeFromJson(o);

	float backingScaleFactor = 1;
	if (json_object_object_get_ex(js, "backingScaleFactor", &o))
		backingScaleFactor = VuoReal_makeFromJson(o);

	return VuoRenderedLayers_makeWithWindow(rootSceneObject, pixelsWide, pixelsHigh, backingScaleFactor, window);
}

/**
 * @ingroup VuoRenderedLayers
 * @see VuoSceneObject_getJson
 */
json_object * VuoRenderedLayers_getJson(const VuoRenderedLayers value)
{
	json_object *js = json_object_new_object();

	json_object *rootSceneObjectObject = VuoSceneObject_getJson(value.rootSceneObject);
	json_object_object_add(js, "rootSceneObject", rootSceneObjectObject);

	json_object *pixelsWideObject = json_object_new_int64(value.pixelsWide);
	json_object_object_add(js, "pixelsWide", pixelsWideObject);

	json_object *pixelsHighObject = json_object_new_int64(value.pixelsHigh);
	json_object_object_add(js, "pixelsHigh", pixelsHighObject);

	json_object *windowObject = VuoWindowReference_getJson(value.window);
	json_object_object_add(js, "window", windowObject);

	json_object *bsfObject = VuoReal_getJson(value.backingScaleFactor);
	json_object_object_add(js, "backingScaleFactor", bsfObject);

	return js;
}

/**
 * @ingroup VuoRenderedLayers
 * @see VuoSceneObject_getSummary
 */
char * VuoRenderedLayers_getSummary(const VuoRenderedLayers value)
{
	char *rootSummary = VuoSceneObject_getSummary(value.rootSceneObject);
	char *windowSummary = VuoWindowReference_getSummary(value.window);

	char *summary = VuoText_format("%lux%lu<br>%s<br>%s", value.pixelsWide, value.pixelsHigh, windowSummary, rootSummary);

	free(windowSummary);
	free(rootSummary);

	return summary;
}
