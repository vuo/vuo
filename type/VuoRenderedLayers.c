/**
 * @file
 * VuoRenderedLayers implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include "VuoRenderedLayers.h"
#include "VuoImageText.h"
#include "VuoSceneText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Window",
					 "description" : "A set of layers, transformed to their final positions for rendering",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoInteraction",
						 "VuoImageText",
						 "VuoRectangle",
						 "VuoSceneObject",
						 "VuoSceneText",
						 "VuoWindowReference",
						 "VuoList_VuoInteraction",
						 "VuoList_VuoSceneObject"
					 ]
				 });
#endif
/// @}

/**
 * @private VuoRenderedLayers fields.
 */
typedef struct
{
	VuoList_VuoInteraction interactions;

	VuoSceneObject rootSceneObject;

	unsigned long int pixelsWide;
	unsigned long int pixelsHigh;
	float backingScaleFactor;
	bool hasRenderingDimensions;

	VuoWindowReference window;
	bool hasWindow;
} VuoRenderedLayers_internal;

/**
 * Forward declaration of private getTransformedLayer function.
 */
bool VuoRenderedLayers_getTransformedLayer2(VuoRenderedLayers_internal *rl, float localToWorldMatrix[16], VuoSceneObject targetObject, VuoPoint2d *layerCenter, VuoPoint2d layerCorners[4]);

/**
 * Frees the memory associated with the object.
 *
 * @threadAny
 */
void VuoRenderedLayers_free(void *renderedLayers)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;

	VuoSceneObject_release(rl->rootSceneObject);
	VuoRelease(rl->interactions);
	VuoRelease(rl->window);

	free(rl);
}


/**
 * Creates a VuoRenderedLayers with the given rendering root layer, rendering dimensions, and no window.
 *
 * @deprecated
 */
VuoRenderedLayers VuoRenderedLayers_make(	VuoSceneObject rootSceneObject,
											unsigned long int pixelsWide,
											unsigned long int pixelsHigh,
											float backingScaleFactor,
											VuoList_VuoInteraction interactions)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)VuoRenderedLayers_makeEmpty();

	rl->rootSceneObject = rootSceneObject;
	VuoSceneObject_retain(rl->rootSceneObject);

	rl->pixelsWide = pixelsWide;
	rl->pixelsHigh = pixelsHigh;
	rl->backingScaleFactor = backingScaleFactor;
	rl->hasRenderingDimensions = true;

	rl->interactions = interactions;
	VuoRetain(rl->interactions);

	return (VuoRenderedLayers)rl;
}

/**
 * Creates a VuoRenderedLayers with the given rendering root layer, rendering dimensions, and window.
 *
 * @deprecated
 */
VuoRenderedLayers VuoRenderedLayers_makeWithWindow(VuoSceneObject rootSceneObject,
												   unsigned long int pixelsWide, unsigned long int pixelsHigh,
												   float backingScaleFactor,
												   VuoWindowReference window,
												   VuoList_VuoInteraction interactions)
{
	VuoRenderedLayers renderedLayers = VuoRenderedLayers_make(rootSceneObject, pixelsWide, pixelsHigh, backingScaleFactor, interactions);
	VuoRenderedLayers_setWindow(renderedLayers, window);
	return renderedLayers;
}

/**
 * Creates a VuoRenderedLayers that doesn't yet store any information about rendered items or the rendering destination.
 * Use `VuoRenderedLayers_set*` to populate the information and `VuoRenderedLayers_get*` to retrieve it.
 */
VuoRenderedLayers VuoRenderedLayers_makeEmpty(void)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)calloc(1, sizeof(VuoRenderedLayers_internal));
	VuoRegister(rl, VuoRenderedLayers_free);

	rl->backingScaleFactor = 1;

	return (VuoRenderedLayers)rl;
}

/**
 * Populates part of the information stored in the VuoRenderedLayers instance. This function can be used with any
 * combination of other `VuoRenderedLayers_set*` functions.
 * Retains the right-hand object (and releases the old object if needed).
 */
void VuoRenderedLayers_setInteractions(VuoRenderedLayers renderedLayers, VuoList_VuoInteraction interactions)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;

	if (rl->interactions)
		VuoRelease(rl->interactions);

	rl->interactions = interactions;
	VuoRetain(rl->interactions);
}

/// @copydoc VuoRenderedLayers_setInteractions
void VuoRenderedLayers_setRootSceneObject(VuoRenderedLayers renderedLayers, VuoSceneObject rootSceneObject)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;

	VuoRelease(rl->rootSceneObject);

	rl->rootSceneObject = rootSceneObject;
	VuoRetain(rl->rootSceneObject);
}

/// @copydoc VuoRenderedLayers_setInteractions
void VuoRenderedLayers_setWindow(VuoRenderedLayers renderedLayers, VuoWindowReference window)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;

	if (rl->window)
		VuoRelease(rl->window);

	rl->hasWindow = true;
	rl->window = window;
	VuoRetain(rl->window);
}

/**
 * Returns the list of interactions (if any).
 */
VuoList_VuoInteraction VuoRenderedLayers_getInteractions(const VuoRenderedLayers renderedLayers)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;
	return rl->interactions;
}

/**
 * Returns the root sceneobject (if any).
 */
VuoSceneObject VuoRenderedLayers_getRootSceneObject(const VuoRenderedLayers renderedLayers)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;
	return rl->rootSceneObject;
}

/**
 * If the requested piece of information has been populated, returns true and passes it through the last argument(s).
 * Otherwise, returns false and leaves the last argument(s) unchanged.
 */
bool VuoRenderedLayers_getRenderingDimensions(const VuoRenderedLayers renderedLayers, unsigned long int *pixelsWide, unsigned long int *pixelsHigh, float *backingScaleFactor)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;

	if (rl->hasRenderingDimensions)
	{
		*pixelsWide = rl->pixelsWide;
		*pixelsHigh = rl->pixelsHigh;
		*backingScaleFactor = rl->backingScaleFactor;
		return true;
	}

	return false;
}

/**
 * @private method for TestVuoLayer.
 */
void VuoRenderedLayers_setRenderingDimensions(const VuoRenderedLayers renderedLayers, unsigned long int pixelsWide, unsigned long int pixelsHigh, float backingScaleFactor)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;

	rl->hasRenderingDimensions = true;
	rl->pixelsWide = pixelsWide;
	rl->pixelsHigh = pixelsHigh;
	rl->backingScaleFactor = backingScaleFactor;
}

/**
 * Outputs the window reference via the second argument, if it's been populated, and returns true.
 * Otherwise, returns false and leaves the second argument unchanged.
 */
bool VuoRenderedLayers_getWindow(const VuoRenderedLayers renderedLayers, VuoWindowReference *window)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;

	if (rl->hasWindow)
	{
		*window = rl->window;
		return true;
	}

	return false;
}

/**
 * Returns true if the window in @a newerRenderedLayers is a change from @a accumulatedRenderedLayers.
 */
bool VuoRenderedLayers_windowChanged(const VuoRenderedLayers accumulatedRenderedLayers, const VuoRenderedLayers newerRenderedLayers)
{
	VuoWindowReference window;
	if (VuoRenderedLayers_getWindow(newerRenderedLayers, &window))
	{
		VuoWindowReference oldWindow;
		bool hasWindow = VuoRenderedLayers_getWindow(accumulatedRenderedLayers, &oldWindow);
		if (! hasWindow || oldWindow != window)
			return true;
	}

	return false;
}

/**
 * Updates @a accumulatedRenderedLayers with the values that are populated in @a newerRenderedLayers.
 * The rest of the values in @a accumulatedRenderedLayers remain unchanged.
 *
 * Sets @a renderingDimensionsChanged to true if the window's width, height, or backing scale factor
 * have changed, false otherwise. These values are checked for changes when updating them, rather than
 * a separate function, so that the checking and updating code can share the same query to the window
 * and thus give consistent results.
 *
 * The interactions in `newerRenderedLayers` are merely passed through to `accumulatedRenderedLayers`
 * (not coalesced), since they are transient.
 */
void VuoRenderedLayers_update(VuoRenderedLayers accumulatedRenderedLayers, const VuoRenderedLayers newerRenderedLayers,
							  bool *renderingDimensionsChanged)
{
	VuoRenderedLayers_internal *arl = (VuoRenderedLayers_internal *)accumulatedRenderedLayers;
	VuoRenderedLayers_internal *nrl = (VuoRenderedLayers_internal *)newerRenderedLayers;

	*renderingDimensionsChanged = false;

	VuoRenderedLayers_setInteractions(accumulatedRenderedLayers, nrl->interactions);

	VuoSceneObject rootSceneObject = VuoRenderedLayers_getRootSceneObject(newerRenderedLayers);
	if (rootSceneObject)
		VuoRenderedLayers_setRootSceneObject(accumulatedRenderedLayers, rootSceneObject);

	VuoWindowReference window;
	if (VuoRenderedLayers_getWindow(newerRenderedLayers, &window))
		VuoRenderedLayers_setWindow(accumulatedRenderedLayers, window);

	if (VuoRenderedLayers_getWindow(accumulatedRenderedLayers, &window))
	{
		VuoInteger pixelsWide;
		VuoInteger pixelsHigh;
		float backingScaleFactor;
		VuoWindowReference_getContentSize(window, &pixelsWide, &pixelsHigh, &backingScaleFactor);

		if (!arl->hasRenderingDimensions ||
				arl->pixelsWide != pixelsWide || arl->pixelsHigh != pixelsHigh || arl->backingScaleFactor != backingScaleFactor)
			*renderingDimensionsChanged = true;

		arl->hasRenderingDimensions = true;
		arl->pixelsWide = pixelsWide;
		arl->pixelsHigh = pixelsHigh;
		arl->backingScaleFactor = backingScaleFactor;
	}
}

/**
 * Finds the layer with the given name, along with its ancestor layers.  The layer may be a group with children but no mesh.
 */
bool VuoRenderedLayers_findLayer(VuoRenderedLayers renderedLayers, VuoText layerName, VuoList_VuoSceneObject ancestorObjects, VuoSceneObject *foundObject)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;
	return VuoSceneObject_find(rl->rootSceneObject, layerName, ancestorObjects, foundObject);
}

/**
 * Finds the layer with the given ID, along with its ancestor layers.  The layer may be a group with children but no mesh.
 */
bool VuoRenderedLayers_findLayerId(VuoRenderedLayers renderedLayers, uint64_t layerId, VuoList_VuoSceneObject ancestorObjects, VuoSceneObject *foundObject)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;
	return VuoSceneObject_findById(rl->rootSceneObject, layerId, ancestorObjects, foundObject);
}

/**
 * Apply a list of sceneobject transforms in reverse order, taking into account realSize objects.
 * Optionally include or omit applying target transform.  Useful in situations where the local
 * transform has already been applied (VuoRenderedLayers_getRect for example).
 */
void VuoRenderedLayers_applyTransforms(	VuoRenderedLayers renderedLayers,
										VuoList_VuoSceneObject ancestorObjects,
										VuoSceneObject targetObject,
										VuoPoint3d* layerCenter3d,
										VuoPoint3d layerCorners3d[4],
										bool applyTargetTransform)
{
	float matrix[16];
	VuoPoint3d center = *layerCenter3d;
	bool isText = VuoSceneObject_getType(targetObject) == VuoSceneObjectSubType_Text
		&& VuoSceneObject_getText(targetObject);

	if (VuoSceneObject_isRealSize(targetObject) || (isText && !VuoSceneObject_shouldTextScaleWithScene(targetObject)))
	{
		// Real-size layer:
		// Apply the layer's transformations to the center point.
		unsigned long ancestorObjectCount = VuoListGetCount_VuoSceneObject(ancestorObjects);

		// apply local transform first
		if(applyTargetTransform)
		{
			VuoTransform_getMatrix(VuoSceneObject_getTransform(targetObject), matrix);
			center = VuoTransform_transformPoint(matrix, center);
		}

		for (unsigned long i = ancestorObjectCount; i >= 1; --i)
		{
			VuoSceneObject ancestorObject = VuoListGetValue_VuoSceneObject(ancestorObjects, i);
			VuoTransform_getMatrix(VuoSceneObject_getTransform(ancestorObject), matrix);
			center = VuoTransform_transformPoint(matrix, center);
		}
	}
	else
	{
		// Scaled layer:
		// Apply the layer's transformations to each of its corner points.
		unsigned long ancestorObjectCount = VuoListGetCount_VuoSceneObject(ancestorObjects);

		// local transform first
		if(applyTargetTransform)
		{
			VuoTransform_getMatrix(VuoSceneObject_getTransform(targetObject), matrix);

			for(int i = 0; i < 4; i++)
				layerCorners3d[i] = VuoTransform_transformPoint(matrix, layerCorners3d[i]);

			center = VuoTransform_transformPoint(matrix, center);
		}

		for (unsigned long i = ancestorObjectCount; i >= 1; --i)
		{
			VuoSceneObject ancestorObject = VuoListGetValue_VuoSceneObject(ancestorObjects, i);
			VuoTransform_getMatrix(VuoSceneObject_getTransform(ancestorObject), matrix);
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
 * vertex positions.  If targetObject does not have a mesh (or is not a quad) and includeChildrenInBounds is false
 * no points are returned.
 */
bool VuoRenderedLayers_getLayerCorners(const VuoSceneObject targetObject, VuoPoint3d layerCorners3d[4])
{
	VuoMesh mesh = VuoSceneObject_getMesh(targetObject);
	if (!mesh)
		return false;

	unsigned int vertexCount;
	float *positions;
	VuoMesh_getCPUBuffers(mesh, &vertexCount, &positions, NULL, NULL, NULL, NULL, NULL);
	if (vertexCount < 3)
		return false;

	// point3d because transformations are carried out using 3d transform functions
	for (int i = 0; i < 4; ++i)
		layerCorners3d[i] = VuoPoint3d_makeFromArray(&positions[i * 3]);

    return true;
}

/**
 * Returns the center of the 4 coordinates (ignoring Z).
 */
VuoPoint3d VuoRenderedLayers_getQuadCenter(VuoPoint3d layerCorners3d[4])
{
	VuoPoint3d c = layerCorners3d[0];

	for(int i = 1; i < 4; i++)
	{
		c.x += layerCorners3d[i].x;
		c.y += layerCorners3d[i].y;
	}

	return VuoPoint3d_multiply(c, .25);
}

/**
 * Helper for @ref VuoRenderedLayers_getRect.
 */
bool VuoRenderedLayers_getRectRecursive(VuoRenderedLayers_internal *rl, float compositeMatrix[16], VuoSceneObject targetObject, VuoRectangle* rect, bool rectIsInitialized)
{
	bool foundRect = rectIsInitialized;
	VuoPoint2d layerCenter;
	VuoPoint2d layerCorners[4];

	// apply targetObject transform
	float localToWorldMatrix[16];
	float modelMatrix[16];

	VuoTransform_getMatrix(VuoSceneObject_getTransform(targetObject), modelMatrix);
	VuoTransform_multiplyMatrices4x4(modelMatrix, compositeMatrix, localToWorldMatrix);

	if (VuoRenderedLayers_getTransformedLayer2(rl, localToWorldMatrix, targetObject, &layerCenter, layerCorners) )
	{
		VuoRectangle thisRect = VuoRenderedLayers_getBoundingBox(layerCorners);

		if(rectIsInitialized)
			*rect = VuoRectangle_union(*rect, thisRect);
		else
			*rect = thisRect;

		foundRect = true;
	}

	VuoList_VuoSceneObject childObjects = VuoSceneObject_getChildObjects(targetObject);
	int children = VuoListGetCount_VuoSceneObject(childObjects);

	for(int i = 1; i <= children; i++)
	{
		VuoSceneObject child = VuoListGetValue_VuoSceneObject(childObjects, i);

		if( VuoRenderedLayers_getRectRecursive(rl, localToWorldMatrix, child, rect, foundRect) )
			foundRect = true;
	}

	return foundRect;
}

/**
 * Get a axis-aligned bounding rect in model space (transformed by `layer`) in Vuo coordinates for a layer and its children.
 */
bool VuoRenderedLayers_getRect(VuoRenderedLayers renderedLayers, VuoSceneObject layer, VuoRectangle* rect)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;

	float identity[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};

	return VuoRenderedLayers_getRectRecursive(rl, identity, layer, rect, false);
}

/**
 * Outputs the center and corner points of the layer with the given name, as transformed in @a renderedLayers.
 *
 * If false is returned, values have not been assigned to `layerCenter` and `layerCorners`.
 */
bool VuoRenderedLayers_getTransformedLayer(
	VuoRenderedLayers renderedLayers,
	VuoList_VuoSceneObject ancestorObjects,
	VuoSceneObject targetObject,
	VuoPoint2d *layerCenter,
	VuoPoint2d layerCorners[4],
	bool includeChildrenInBounds)
{
	for (int i = 0; i < 4; ++i)
		layerCorners[i] = (VuoPoint2d){NAN,NAN};

	// Get the layer's corner points.
	VuoPoint3d layerCorners3d[4];

	if(includeChildrenInBounds)
	{
		VuoRectangle rect;

		if(!VuoRenderedLayers_getRect(renderedLayers, targetObject, &rect))
			return false;

		VuoPoint2d c = rect.center;
		VuoPoint2d e = VuoPoint2d_multiply(rect.size, .5);

		layerCorners3d[0] = VuoPoint3d_make( c.x - e.x, c.y - e.y, 0. );
		layerCorners3d[1] = VuoPoint3d_make( c.x + e.x, c.y - e.y, 0. );
		layerCorners3d[2] = VuoPoint3d_make( c.x - e.x, c.y + e.y, 0. );
		layerCorners3d[3] = VuoPoint3d_make( c.x + e.x, c.y + e.y, 0. );
	}
	else
	{
		if( !VuoRenderedLayers_getLayerCorners(targetObject, layerCorners3d) )
			return false;
	}

	bool isText = VuoSceneObject_getType(targetObject) == VuoSceneObjectSubType_Text
		&& VuoSceneObject_getText(targetObject);

	// if includeChildren is true VuoRenderedLayers_getRect will have already applied scale
	VuoShader shader = VuoSceneObject_getShader(targetObject);
	if (!includeChildrenInBounds && !isText && shader)
	{
		for (int i = 0; i < 4; ++i)
		{
			layerCorners3d[i].x *= shader->objectScale;
			layerCorners3d[i].y *= shader->objectScale;
		}
	}

	// Transform the layer to the rendered layers' coordinate space.
	VuoPoint3d layerCenter3d = VuoRenderedLayers_getQuadCenter(layerCorners3d);

	VuoRenderedLayers_applyTransforms(renderedLayers, ancestorObjects, targetObject, &layerCenter3d, layerCorners3d, !includeChildrenInBounds);

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
	VuoShader shader = VuoSceneObject_getShader(targetObject);
	if (!shader)
		return false;

	VuoPoint3d tp = VuoPoint3d_make(point.x, point.y, 0);

	tp.x *= shader->objectScale;
	tp.y *= shader->objectScale;

	// Transform the layer to the rendered layers' coordinate space.
	VuoRenderedLayers_applyTransforms(renderedLayers, ancestorObjects, targetObject, &tp, NULL, true);

	*transformedPoint = VuoPoint2d_make(tp.x, tp.y);

	return true;
}

/**
 * Transform a world point to targetObject local coordinates. Wraps VuoRenderedLayers_findLayer & VuoRenderedLayers_getInverseTransformedPoint.
 */
bool VuoRenderedLayers_getInverseTransformedPointLayer(VuoRenderedLayers renderedLayers, uint64_t targetLayer, VuoPoint2d point, VuoPoint2d* localPoint)
{
	VuoList_VuoSceneObject ancestors = VuoListCreate_VuoSceneObject();
	VuoLocal(ancestors);
	VuoSceneObject target;

	if( VuoRenderedLayers_findLayerId(renderedLayers, targetLayer, ancestors, &target) )
	{
		VuoSceneObject dummy = VuoSceneObject_makeEmpty();

		if(VuoRenderedLayers_getInverseTransformedPoint(renderedLayers, ancestors, dummy, point, localPoint))
			return true;
	}

	return false;
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
	VuoTransform_getMatrix(VuoSceneObject_getTransform(targetObject), localToWorldMatrix);

	int count = VuoListGetCount_VuoSceneObject(ancestorObjects);

	for(int i = count; i > 0; i--)
	{
		// apply targetObject transform
		VuoSceneObject node = VuoListGetValue_VuoSceneObject(ancestorObjects, i);

		VuoTransform_copyMatrix4x4(localToWorldMatrix, tmp);
		VuoTransform_getMatrix(VuoSceneObject_getTransform(node), modelMatrix);
		VuoTransform_multiplyMatrices4x4(modelMatrix, tmp, localToWorldMatrix);
	}

	float worldToLocalMatrix[16];
	VuoTransform_invertMatrix4x4(localToWorldMatrix, worldToLocalMatrix);

	VuoPoint3d point3d = VuoPoint3d_make(point.x, point.y, 0);
	VuoPoint3d invPoint = VuoTransform_transformPoint(worldToLocalMatrix, point3d);

	VuoShader shader = VuoSceneObject_getShader(targetObject);
	if (shader)
	{
		invPoint.x /= shader->objectScale;
		invPoint.y /= shader->objectScale;
	}

	*inverseTransformedPoint = VuoPoint2d_make(invPoint.x, invPoint.y);

	return true;
}

/**
 * Outputs the center and corner points of the layer.  localToWorldMatrix should be composite all ancestor transforms and targetObject.transform.
 */
bool VuoRenderedLayers_getTransformedLayer2(VuoRenderedLayers_internal *renderedLayers, float localToWorldMatrix[16], VuoSceneObject targetObject, VuoPoint2d *layerCenter, VuoPoint2d layerCorners[4])
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;

	// Get the layer's corner points.
	VuoPoint3d layerCorners3d[4];

	if( !VuoRenderedLayers_getLayerCorners(targetObject, layerCorners3d) )
		return false;

	bool isText = VuoSceneObject_getType(targetObject) == VuoSceneObjectSubType_Text
		&& VuoSceneObject_getText(targetObject);

	VuoShader shader = VuoSceneObject_getShader(targetObject);
	if (!shader && !isText)
		return false;

	if(!isText)
	{
		for (int i = 0; i < 4; ++i)
		{
			layerCorners3d[i].x *= shader->objectScale;
			layerCorners3d[i].y *= shader->objectScale;
		}
	}

	// Transform the layer to the rendered layers' coordinate space.
	VuoPoint3d center = VuoPoint3d_make(0,0,0);

	if (VuoSceneObject_isRealSize(targetObject) || isText)
	{
		// Real-size layer:
		// Apply the layer's transformations to the center point.
		center = VuoTransform_transformPoint(localToWorldMatrix, center);

		float widthScale = 1, heightScale = 1;

		// Scale the layer's corner points to the rendered layers' coordinate space.
		if(isText)
		{
			float verticalScale = 1.;
			float rotationZ = 0.;
			if (VuoSceneObject_shouldTextScaleWithScene(targetObject))
			{
				VuoTransform transform = VuoTransform_makeFromMatrix4x4(localToWorldMatrix);
				widthScale    = transform.scale.x;
				verticalScale = transform.scale.y / transform.scale.x;
				rotationZ     = VuoTransform_getEuler(transform).z;
			}

			VuoPoint2d size = VuoRenderedLayers_getTextSize((VuoRenderedLayers)renderedLayers, VuoSceneObject_getText(targetObject), VuoSceneObject_getTextFont(targetObject), VuoSceneObject_shouldTextScaleWithScene(targetObject), verticalScale, rotationZ, VuoSceneObject_getTextWrapWidth(targetObject), true);

			center.xy += VuoSceneText_getAnchorOffset(targetObject, verticalScale, rotationZ, VuoSceneObject_getTextWrapWidth(targetObject),
				VuoSceneObject_shouldTextScaleWithScene(targetObject) ? VuoGraphicsWindowDefaultWidth * rl->backingScaleFactor : rl->pixelsWide,
				rl->backingScaleFactor);

			widthScale *= size.x;
			heightScale *= size.y;
		}
		else
		{
			VuoImage image = VuoShader_getUniform_VuoImage(shader, "texture");

			widthScale = 2. * (float)image->pixelsWide / rl->pixelsWide;
			heightScale = widthScale * (float)image->pixelsHigh / (float)image->pixelsWide;

			VuoReal combinedScaleFactor = 1;

			if (VuoSceneObject_shouldPreservePhysicalSize(targetObject))
				combinedScaleFactor = rl->backingScaleFactor / image->scaleFactor;

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

	*layerCenter = VuoPoint2d_make(center.x, center.y);

	return true;
}

/**
 * Outputs the size, in Vuo Coordinates, of a text layer (real-size or scaled).
 */
VuoPoint2d VuoRenderedLayers_getTextSize(VuoRenderedLayers renderedLayers, VuoText text, VuoFont font, bool scaleWithScene, float verticalScale, float rotationZ, float wrapWidth, bool includeTrailingWhiteSpace)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;

	// If we don't know the size of the render destination,
	// we can't calculate the size of real-size text.
	if (!scaleWithScene && (rl->pixelsWide < 1 || rl->pixelsHigh < 1))
		return VuoPoint2d_make(0, 0);

	VuoRectangle textBounds = VuoImage_getTextRectangle(text, font, rl->backingScaleFactor, verticalScale, rotationZ, wrapWidth, includeTrailingWhiteSpace);

	float w = textBounds.size.x;
	float h = textBounds.size.y;

	VuoPoint2d size;
	if (scaleWithScene)
		size.x = (w / (float)(VuoGraphicsWindowDefaultWidth * rl->backingScaleFactor)) * 2;
	else
		size.x = (w / (float)rl->pixelsWide) * 2;
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
		if (isnan(layerCorners[i].x) || isnan(layerCorners[i].y))
			goto nan;

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
nan:
	return (VuoRectangle){
		(VuoPoint2d){NAN,NAN},
		(VuoPoint2d){NAN,NAN}
	};
}

/**
 * Returns true if the given point is within (or on the boundary) of the quad with the given corner points.
 */
bool VuoRenderedLayers_isPointInQuad(VuoPoint2d corners[4], VuoPoint2d point)
{
	// Split the quad into two triangles with points specified counter-clockwise, and check each triangle.
	// https://stackoverflow.com/questions/2049582/how-to-determine-a-point-in-a-triangle
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
bool VuoRenderedLayers_isPointInLayerRecursive(VuoRenderedLayers_internal *rl, float compositeMatrix[16], VuoSceneObject targetObject, VuoPoint2d point)
{
	VuoPoint2d layerCenter;
	VuoPoint2d layerCorners[4];

	// apply targetObject transform
	float localToWorldMatrix[16];
	float modelMatrix[16];

	VuoTransform_getMatrix(VuoSceneObject_getTransform(targetObject), modelMatrix);
	VuoTransform_multiplyMatrices4x4(modelMatrix, compositeMatrix, localToWorldMatrix);

	if (VuoRenderedLayers_getTransformedLayer2(rl, localToWorldMatrix, targetObject, &layerCenter, layerCorners))
	{
		if( VuoRenderedLayers_isPointInQuad(layerCorners, point) )
			return true;
	}

	VuoList_VuoSceneObject childObjects = VuoSceneObject_getChildObjects(targetObject);
	int children = VuoListGetCount_VuoSceneObject(childObjects);

	for(int i = 1; i <= children; i++)
	{
		VuoSceneObject child = VuoListGetValue_VuoSceneObject(childObjects, i);
		if( VuoRenderedLayers_isPointInLayerRecursive(rl, localToWorldMatrix, child, point) )
			return true;
	}

	return false;
}

/**
 * Returns true if the given point is within (or on the boundary) of the layer with the given name, or any of its children.
 *
 * @a point should be in the same coordinate space as @a renderedLayers.
 */
bool VuoRenderedLayers_isPointInLayer(VuoRenderedLayers renderedLayers, VuoText layerName, VuoPoint2d point)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;

	float aspect = (double)rl->pixelsHigh / rl->pixelsWide;

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
		VuoTransform_getMatrix(VuoSceneObject_getTransform(o), modelMatrix);
		VuoTransform_multiplyMatrices4x4(modelMatrix, composite, tmp);
		VuoTransform_copyMatrix4x4(tmp, composite);
	}

	return VuoRenderedLayers_isPointInLayerRecursive(rl, composite, layer, point);
}

/**
 * Returns true if the given point is within (or on the boundary) of the layer with the given id, or any of its children.
 *
 * @a point should be in the same coordinate space as @a renderedLayers.
 */
bool VuoRenderedLayers_isPointInLayerId(VuoRenderedLayers renderedLayers, uint64_t layerId, VuoPoint2d point)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;

	float aspect = (double)rl->pixelsHigh / rl->pixelsWide;

	if( point.x < -1 || point.x > 1 || point.y < -aspect || point.y > aspect )
		return false;

	VuoSceneObject layer;
	VuoList_VuoSceneObject ancestors = VuoListCreate_VuoSceneObject();
	VuoLocal(ancestors);

	if (!VuoSceneObject_findById(rl->rootSceneObject, layerId, ancestors, &layer))
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
		VuoTransform_getMatrix(VuoSceneObject_getTransform(o), modelMatrix);
		VuoTransform_multiplyMatrices4x4(modelMatrix, composite, tmp);
		VuoTransform_copyMatrix4x4(tmp, composite);
	}

	return VuoRenderedLayers_isPointInLayerRecursive(rl, composite, layer, point);
}

/**
 * Iterates through all interactions and tests for input events over the layer of layerName.
 */
void VuoRenderedLayers_getEventsInLayer(VuoRenderedLayers renderedLayers,
										uint64_t id,
										bool *anyHover,
										bool *anyPressed,
										bool *anyReleased,
										bool *anyClicked)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;

	*anyHover = false;
	*anyPressed = false;
	*anyReleased = false;
	*anyClicked = false;

	if (rl->interactions == NULL || VuoListGetCount_VuoInteraction(rl->interactions) < 1)
		return;

	for (int i = 1; i <= VuoListGetCount_VuoInteraction(rl->interactions); i++)
	{
		VuoInteraction it = VuoListGetValue_VuoInteraction(rl->interactions, i);

		if( VuoRenderedLayers_isPointInLayerId(renderedLayers, id, it.position) )
		{
			*anyHover = true;

			if(it.isPressed)
				*anyPressed = true;

			if(	it.type == VuoInteractionType_Click )
				*anyClicked = true;

			if( it.type == VuoInteractionType_Release )
				*anyReleased = true;
		}
	}
}

/**
 * @ingroup VuoRenderedLayers
 * @see VuoSceneObject_makeFromJson
 */
VuoRenderedLayers VuoRenderedLayers_makeFromJson(json_object * js)
{
	json_object *o = NULL;

	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)VuoRenderedLayers_makeEmpty();

	if (json_object_object_get_ex(js, "rootSceneObject", &o))
	{
		rl->rootSceneObject = VuoSceneObject_makeFromJson(o);
		VuoSceneObject_retain(rl->rootSceneObject);
	}

	if (json_object_object_get_ex(js, "pixelsWide", &o))
		rl->pixelsWide = json_object_get_int64(o);

	if (json_object_object_get_ex(js, "pixelsHigh", &o))
		rl->pixelsHigh = json_object_get_int64(o);

	if (rl->pixelsWide && rl->pixelsHigh)
		rl->hasRenderingDimensions = true;

	if (json_object_object_get_ex(js, "backingScaleFactor", &o))
		rl->backingScaleFactor = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "interactions", &o))
	{
		rl->interactions = VuoList_VuoInteraction_makeFromJson(o);
		VuoRetain(rl->interactions);
	}

	if (json_object_object_get_ex(js, "window", &o))
	{
		rl->hasWindow = true;
		rl->window = VuoWindowReference_makeFromJson(o);
		VuoRetain(rl->window);
	}

	return (VuoRenderedLayers)rl;
}

/**
 * @ingroup VuoRenderedLayers
 * @see VuoSceneObject_getJson
 */
json_object * VuoRenderedLayers_getJson(const VuoRenderedLayers renderedLayers)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;

	json_object *js = json_object_new_object();

	if (VuoSceneObject_isPopulated(rl->rootSceneObject))
	{
		json_object *rootSceneObjectObject = VuoSceneObject_getJson(rl->rootSceneObject);
		json_object_object_add(js, "rootSceneObject", rootSceneObjectObject);
	}

	if (rl->hasRenderingDimensions)
	{
		json_object *pixelsWideObject = json_object_new_int64(rl->pixelsWide);
		json_object_object_add(js, "pixelsWide", pixelsWideObject);

		json_object *pixelsHighObject = json_object_new_int64(rl->pixelsHigh);
		json_object_object_add(js, "pixelsHigh", pixelsHighObject);

		json_object *bsfObject = VuoReal_getJson(rl->backingScaleFactor);
		json_object_object_add(js, "backingScaleFactor", bsfObject);
	}

	if (rl->hasWindow)
	{
		json_object *windowObject = VuoWindowReference_getJson(rl->window);
		json_object_object_add(js, "window", windowObject);
	}

	if (rl->interactions)
	{
		json_object *interactionObj = VuoList_VuoInteraction_getJson(rl->interactions);
		json_object_object_add(js, "interactions", interactionObj);
	}

	return js;
}

/**
 * @ingroup VuoRenderedLayers
 * @see VuoSceneObject_getSummary
 */
char * VuoRenderedLayers_getSummary(const VuoRenderedLayers renderedLayers)
{
	VuoRenderedLayers_internal *rl = (VuoRenderedLayers_internal *)renderedLayers;

	char *windowSummary = NULL;
	if (rl->hasWindow)
		windowSummary = VuoText_format("<p>%s</p>", VuoWindowReference_getSummary(rl->window));

	char *sizeSummary = NULL;
	if (rl->hasRenderingDimensions)
		sizeSummary = VuoText_format("<p>Size: %lu×%lu @ %gx</p>", rl->pixelsWide, rl->pixelsHigh, rl->backingScaleFactor);

	char *layersSummary = NULL;
	if (VuoSceneObject_isPopulated(rl->rootSceneObject))
		layersSummary = VuoText_format("<p>%s</p>", VuoSceneObject_getSummary(rl->rootSceneObject));

	char *summary = VuoText_format("%s%s%s",
		windowSummary ? windowSummary : "",
		sizeSummary ? sizeSummary : "",
		layersSummary ? layersSummary : "");

	free(windowSummary);
	free(sizeSummary);
	free(layersSummary);

	return summary;
}
