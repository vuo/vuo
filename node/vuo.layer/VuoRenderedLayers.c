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
#include "VuoImageText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Rendered Layers",
					 "description" : "A set of layers, transformed to their final positions for rendering",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoLayer",
						 "VuoImageText",
						 "VuoSceneObject",
						 "VuoWindowReference",
						 "VuoList_VuoSceneObject"
					 ]
				 });
#endif
/// @}

/**
 * Forward declaration of private getTransformedLayer function.
 */
bool VuoRenderedLayers_getTransformedLayer2(VuoRenderedLayers renderedLayers, float localToWorldMatrix[16], VuoSceneObject targetObject, VuoPoint2d *layerCenter, VuoPoint2d layerCorners[4]);

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
 * Finds the layer with the given name, along with its ancestor layers.  The layer may be a group with children but no mesh.
 */
bool VuoRenderedLayers_findLayer(VuoRenderedLayers renderedLayers, VuoText layerName, VuoList_VuoSceneObject ancestorObjects, VuoSceneObject *foundObject)
{
	return VuoSceneObject_find(renderedLayers.rootSceneObject, layerName, ancestorObjects, foundObject);
}

/**
 * Apply a list of sceneobject transforms in reverse order, taking into account realSize objects.
 */
void VuoRenderedLayers_applyTransforms(	VuoRenderedLayers renderedLayers,
										VuoList_VuoSceneObject ancestorObjects,
										VuoSceneObject targetObject,
										VuoPoint3d* layerCenter3d,
										VuoPoint3d layerCorners3d[4])
{
	float matrix[16];
	VuoPoint3d center = VuoPoint3d_make(0,0,0);
	bool isText = targetObject.type == VuoSceneObjectSubType_Text && targetObject.text != NULL;

	if (targetObject.isRealSize || isText)
	{
		// Real-size layer:
		// Apply the layer's transformations to the center point.
		unsigned long ancestorObjectCount = VuoListGetCount_VuoSceneObject(ancestorObjects);

		// apply local transform first
		VuoTransform_getMatrix(targetObject.transform, matrix);
		center = VuoTransform_transformPoint(matrix, center);

		for (unsigned long i = ancestorObjectCount; i >= 1; --i)
		{
			VuoSceneObject ancestorObject = VuoListGetValue_VuoSceneObject(ancestorObjects, i);
			VuoTransform_getMatrix(ancestorObject.transform, matrix);
			center = VuoTransform_transformPoint(matrix, center);
		}
		if(layerCorners3d != NULL)
		{
			float widthScale = 1, heightScale = 1;

			// Scale the layer's corner points to the rendered layers' coordinate space.
			if(isText)
			{
				VuoText text = targetObject.text;
				// getTextSize handles scale factors
				VuoPoint2d size = VuoRenderedLayers_getTextSize(renderedLayers, text, targetObject.font, true);
				widthScale = size.x;
				heightScale = size.y;
			}
			else
			{
				VuoImage image = VuoShader_getUniform_VuoImage(targetObject.shader, "texture");
				widthScale = 2. * (float)image->pixelsWide / (float)renderedLayers.pixelsWide;
				heightScale = widthScale * (float)image->pixelsHigh / (float)image->pixelsWide;
				VuoReal combinedScaleFactor = 1;
				if (targetObject.preservePhysicalSize)
					combinedScaleFactor = renderedLayers.backingScaleFactor / image->scaleFactor;
				widthScale  *= combinedScaleFactor;
				heightScale *= combinedScaleFactor;
			}

			for (int i = 0; i < 4; ++i)
			{
				layerCorners3d[i].x *= widthScale;
				layerCorners3d[i].y *= heightScale;
			}

			// Move the layer's corner points to match the center point.
			for (int i = 0; i < 4; ++i)
			{
				layerCorners3d[i].x += center.x;
				layerCorners3d[i].y += center.y;
			}
		}
	}
	else
	{
		// Scaled layer:
		// Apply the layer's transformations to each of its corner points.
		unsigned long ancestorObjectCount = VuoListGetCount_VuoSceneObject(ancestorObjects);

		// local transform first
		VuoTransform_getMatrix(targetObject.transform, matrix);
		for(int i = 0; i < 4; i++)
			layerCorners3d[i] = VuoTransform_transformPoint(matrix, layerCorners3d[i]);
		center = VuoTransform_transformPoint(matrix, center);

		for (unsigned long i = ancestorObjectCount; i >= 1; --i)
		{
			VuoSceneObject ancestorObject = VuoListGetValue_VuoSceneObject(ancestorObjects, i);
			VuoTransform_getMatrix(ancestorObject.transform, matrix);
			if(layerCorners3d != NULL)
			{
				for (int i = 0; i < 4; ++i)
					layerCorners3d[i] = VuoTransform_transformPoint(matrix, layerCorners3d[i]);
			}
			center = VuoTransform_transformPoint(matrix, center);
		}
	}

	*layerCenter3d = center;
}

/**
 * Returns a set of 4 points representing the corners of targetObject.  If targetObject is a quad, this will be the
 * vertex positions.  If targetObject does not have a mesh (or is not a quad) no points are returned.
 */
bool VuoRenderedLayers_getLayerCorners(const VuoSceneObject targetObject, VuoPoint3d layerCorners3d[4])
{
	if( targetObject.mesh == NULL ||
		targetObject.mesh->submeshCount < 1 ||
		targetObject.mesh->submeshes[0].vertexCount < 3 )
		return false;

	VuoSubmesh layerQuad = targetObject.mesh->submeshes[0];

	// point3d because transformations are carried out using 3d transform functions
	layerCorners3d[0] = VuoPoint3d_make(layerQuad.positions[0].x, layerQuad.positions[0].y, 0);
	layerCorners3d[1] = VuoPoint3d_make(layerQuad.positions[1].x, layerQuad.positions[1].y, 0);
	layerCorners3d[2] = VuoPoint3d_make(layerQuad.positions[2].x, layerQuad.positions[2].y, 0);
	layerCorners3d[3] = VuoPoint3d_make(layerQuad.positions[3].x, layerQuad.positions[3].y, 0);

	return true;
}

/**
 * Outputs the center and corner points of the layer with the given name, as transformed in @a renderedLayers.
 */
bool VuoRenderedLayers_getTransformedLayer(VuoRenderedLayers renderedLayers, VuoList_VuoSceneObject ancestorObjects, VuoSceneObject targetObject, VuoPoint2d *layerCenter, VuoPoint2d layerCorners[4])
{
	// Get the layer's corner points.
	VuoPoint3d layerCorners3d[4];

	if( !VuoRenderedLayers_getLayerCorners(targetObject, layerCorners3d) )
		return false;

	bool isText = targetObject.type = VuoSceneObjectSubType_Text && targetObject.text != NULL;

	if(targetObject.shader == NULL && !isText)
		return false;

	if(!isText)
	{
		for (int i = 0; i < 4; ++i)
		{
			layerCorners3d[i].x *= targetObject.shader->objectScale;
			layerCorners3d[i].y *= targetObject.shader->objectScale;
		}
	}

	// Transform the layer to the rendered layers' coordinate space.
	VuoPoint3d layerCenter3d = VuoPoint3d_make(0,0,0);

	VuoRenderedLayers_applyTransforms(renderedLayers, ancestorObjects, targetObject, &layerCenter3d, layerCorners3d);

	for (int i = 0; i < 4; ++i)
		layerCorners[i] = VuoPoint2d_make(layerCorners3d[i].x, layerCorners3d[i].y);

	*layerCenter = VuoPoint2d_make(layerCenter3d.x, layerCenter3d.y);

	return true;
}

/**
 * Outputs a point as transformed by ancestorObjects and targetObject.
 */
bool VuoRenderedLayers_getTransformedPoint(VuoRenderedLayers renderedLayers, VuoList_VuoSceneObject ancestorObjects, VuoSceneObject targetObject, VuoPoint2d point, VuoPoint2d *transformedPoint)
{
	if(targetObject.shader == NULL)
		return false;

	VuoPoint3d tp = VuoPoint3d_make(point.x, point.y, 0);

	tp.x *= targetObject.shader->objectScale;
	tp.y *= targetObject.shader->objectScale;

	// Transform the layer to the rendered layers' coordinate space.
	VuoRenderedLayers_applyTransforms(renderedLayers, ancestorObjects, targetObject, &tp, NULL);

	*transformedPoint = VuoPoint2d_make(tp.x, tp.y);

	return true;
}

/**
 * Transform point from world to local coordinates.
 */
bool VuoRenderedLayers_getInverseTransformedPoint(VuoRenderedLayers renderedLayers, VuoList_VuoSceneObject ancestorObjects, VuoSceneObject targetObject, VuoPoint2d point, VuoPoint2d *inverseTransformedPoint)
{
	float localToWorldMatrix[16];
	float tmp[16];
	float modelMatrix[16];

	// start off with the targetObject matrix, then work up
	VuoTransform_getMatrix(targetObject.transform, localToWorldMatrix);

	int count = VuoListGetCount_VuoSceneObject(ancestorObjects);

	for(int i = count; i > 0; i--)
	{
		// apply targetObject transform
		VuoSceneObject node = VuoListGetValue_VuoSceneObject(ancestorObjects, i);

		VuoTransform_copyMatrix4x4(localToWorldMatrix, tmp);
		VuoTransform_getMatrix(node.transform, modelMatrix);
		VuoTransform_multiplyMatrices4x4(modelMatrix, tmp, localToWorldMatrix);
	}

	float worldToLocalMatrix[16];
	VuoTransform_invertMatrix4x4(localToWorldMatrix, worldToLocalMatrix);

	VuoPoint3d point3d = VuoPoint3d_make(point.x, point.y, 0);
	VuoPoint3d invPoint = VuoTransform_transformPoint(worldToLocalMatrix, point3d);

	if(targetObject.shader != NULL)
	{
		invPoint.x /= targetObject.shader->objectScale;
		invPoint.y /= targetObject.shader->objectScale;
	}

	*inverseTransformedPoint = VuoPoint2d_make(invPoint.x, invPoint.y);

	return true;
}

/**
 * Helper for @ref VuoRenderedLayers_getRect.
 */
bool VuoRenderedLayers_getRectRecursive(VuoRenderedLayers renderedLayers, float compositeMatrix[16], VuoSceneObject targetObject, VuoRectangle* rect, bool rectIsInitialized)
{
	bool foundRect = rectIsInitialized;
	VuoPoint2d layerCenter;
	VuoPoint2d layerCorners[4];

	// apply targetObject transform
	float localToWorldMatrix[16];
	float modelMatrix[16];

	VuoTransform_getMatrix(targetObject.transform, modelMatrix);
	VuoTransform_multiplyMatrices4x4(modelMatrix, compositeMatrix, localToWorldMatrix);

	if( VuoRenderedLayers_getTransformedLayer2(renderedLayers, localToWorldMatrix, targetObject, &layerCenter, layerCorners) )
	{
		VuoRectangle thisRect = VuoRenderedLayers_getBoundingBox(layerCorners);

		if(rectIsInitialized)
			*rect = VuoPoint2d_rectangleUnion(*rect, thisRect);
		else
			*rect = thisRect;

		foundRect = true;
	}

	int children = VuoListGetCount_VuoSceneObject(targetObject.childObjects);

	for(int i = 1; i <= children; i++)
	{
		VuoSceneObject child = VuoListGetValue_VuoSceneObject(targetObject.childObjects, i);

		if( VuoRenderedLayers_getRectRecursive(renderedLayers, localToWorldMatrix, child, rect, foundRect) )
			foundRect = true;
	}

	return foundRect;
}

/**
 * Get a axis-aligned bounding rect in model space in Vuo coordinates for a layer and its children.
 */
bool VuoRenderedLayers_getRect(VuoRenderedLayers renderedLayers, VuoLayer layer, VuoRectangle* rect)
{
	float identity[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};

	return VuoRenderedLayers_getRectRecursive(renderedLayers, identity, layer.sceneObject, rect, false);
}

/**
 * Outputs the center and corner points of the layer.  localToWorldMatrix should be composite all ancestor transforms and targetObject.transform.
 */
bool VuoRenderedLayers_getTransformedLayer2(VuoRenderedLayers renderedLayers, float localToWorldMatrix[16], VuoSceneObject targetObject, VuoPoint2d *layerCenter, VuoPoint2d layerCorners[4])
{
	// Get the layer's corner points.
	VuoPoint3d layerCorners3d[4];

	if( !VuoRenderedLayers_getLayerCorners(targetObject, layerCorners3d) )
		return false;

	bool isText = targetObject.type == VuoSceneObjectSubType_Text && targetObject.text != NULL;

	if(targetObject.shader == NULL && !isText)
		return false;

	if(!isText)
	{
		for (int i = 0; i < 4; ++i)
		{
			layerCorners3d[i].x *= targetObject.shader->objectScale;
			layerCorners3d[i].y *= targetObject.shader->objectScale;
		}
	}

	// Transform the layer to the rendered layers' coordinate space.
	VuoPoint3d layerCenter3d = VuoPoint3d_make(0,0,0);
	VuoPoint3d center = VuoPoint3d_make(0,0,0);

	if (targetObject.isRealSize || isText)
	{
		// Real-size layer:
		// Apply the layer's transformations to the center point.
		center = VuoTransform_transformPoint(localToWorldMatrix, center);

		float widthScale = 1, heightScale = 1;

		// Scale the layer's corner points to the rendered layers' coordinate space.
		if(isText)
		{
			VuoText text = targetObject.text;
			VuoPoint2d size = VuoRenderedLayers_getTextSize(renderedLayers, text, targetObject.font, true);
			widthScale = size.x;
			heightScale = size.y;
		}
		else
		{
			VuoImage image = VuoShader_getUniform_VuoImage(targetObject.shader, "texture");

			widthScale = 2. * (float)image->pixelsWide / (float)renderedLayers.pixelsWide;
			heightScale = widthScale * (float)image->pixelsHigh / (float)image->pixelsWide;

			VuoReal combinedScaleFactor = 1;

			if (targetObject.preservePhysicalSize)
				combinedScaleFactor = renderedLayers.backingScaleFactor / image->scaleFactor;

			widthScale  *= combinedScaleFactor;
			heightScale *= combinedScaleFactor;
		}

		for (int i = 0; i < 4; ++i)
		{
			layerCorners3d[i].x *= widthScale;
			layerCorners3d[i].y *= heightScale;

			layerCorners3d[i].x += center.x;
			layerCorners3d[i].y += center.y;
		}
	}
	else
	{
		// Scaled layer:
		// Apply the layer's transformations to each of its corner points.
		for (int i = 0; i < 4; ++i)
			layerCorners3d[i] = VuoTransform_transformPoint(localToWorldMatrix, layerCorners3d[i]);

		center = VuoTransform_transformPoint(localToWorldMatrix, center);
	}

	for (int i = 0; i < 4; ++i)
		layerCorners[i] = VuoPoint2d_make(layerCorners3d[i].x, layerCorners3d[i].y);

	*layerCenter = VuoPoint2d_make(layerCenter3d.x, layerCenter3d.y);

	return true;
}

/**
 * Outputs the size in Vuo coordinates of a real-size text layer.
 */
VuoPoint2d VuoRenderedLayers_getTextSize(VuoRenderedLayers renderedLayers, VuoText text, VuoFont font, bool includeTrailingWhiteSpace)
{
	// instead of letting function divide by zero and return -INF just send back 0.
	if(renderedLayers.pixelsWide < 1 || renderedLayers.pixelsHigh < 1)
		return VuoPoint2d_make(0, 0);

	VuoRectangle textBounds = VuoImage_getTextRectangle(text, font, includeTrailingWhiteSpace);

	float w = textBounds.size.x * renderedLayers.backingScaleFactor;
	float h = textBounds.size.y * renderedLayers.backingScaleFactor;

	VuoPoint2d size;
	size.x = (w / (float)renderedLayers.pixelsWide) * 2;
	size.y = size.x * (h / w);

	return size;
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
 * Is point in this object, or any of it's children?
 */
bool VuoRenderedLayers_isPointInLayerRecursive(VuoRenderedLayers renderedLayers, float compositeMatrix[16], VuoSceneObject targetObject, VuoPoint2d point)
{
	VuoPoint2d layerCenter;
	VuoPoint2d layerCorners[4];

	// apply targetObject transform
	float localToWorldMatrix[16];
	float modelMatrix[16];

	VuoTransform_getMatrix(targetObject.transform, modelMatrix);
	VuoTransform_multiplyMatrices4x4(modelMatrix, compositeMatrix, localToWorldMatrix);

	if( VuoRenderedLayers_getTransformedLayer2(renderedLayers, localToWorldMatrix, targetObject, &layerCenter, layerCorners) )
	{
		if( VuoRenderedLayers_isPointInQuad(layerCorners, point) )
			return true;
	}

	int children = VuoListGetCount_VuoSceneObject(targetObject.childObjects);

	for(int i = 1; i <= children; i++)
	{
		VuoSceneObject child = VuoListGetValue_VuoSceneObject(targetObject.childObjects, i);
		if( VuoRenderedLayers_isPointInLayerRecursive(renderedLayers, localToWorldMatrix, child, point) )
			return true;
	}

	return false;
}

/**
 * Returns true if the given point is within (or on the boundary) of the layer with the given name, or any of it's children.
 *
 * @a point should be in the same coordinate space as @a renderedLayers.
 */
bool VuoRenderedLayers_isPointInLayer(VuoRenderedLayers renderedLayers, VuoText layerName, VuoPoint2d point)
{
	float aspect = (double) renderedLayers.pixelsHigh / (double) renderedLayers.pixelsWide;

	if( point.x < -1 || point.x > 1 || point.y < -aspect || point.y > aspect )
		return false;

	VuoSceneObject layer;
	VuoList_VuoSceneObject ancestors = VuoListCreate_VuoSceneObject();
	VuoLocal(ancestors);

	if(!VuoRenderedLayers_findLayer(renderedLayers, layerName, ancestors, &layer))
		return false;

	float modelMatrix[16], tmp[16];
	float composite[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};

	for(int i = 1; i <= VuoListGetCount_VuoSceneObject(ancestors); i++)
	{
		VuoSceneObject o = VuoListGetValue_VuoSceneObject(ancestors, i);
		VuoTransform_getMatrix(o.transform, modelMatrix);
		VuoTransform_multiplyMatrices4x4(modelMatrix, composite, tmp);
		VuoTransform_copyMatrix4x4(tmp, composite);
	}

	return VuoRenderedLayers_isPointInLayerRecursive(renderedLayers, composite, layer, point);
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
