/**
 * @file
 * VuoRenderedLayers C type definition.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoSceneObject.h"
#include "VuoWindowReference.h"
#include "VuoInteraction.h"
#include "VuoList_VuoInteraction.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoRenderedLayers VuoRenderedLayers
 * A snapshot of a window's state.
 * @{
 */

/**
 * A snapshot of a window's state: the window itself and its most recent pointer interactions, dimensions, and layers.
 *
 * @version200Changed{VuoRenderedLayers is now an opaque, heap-allocated type.
 * Please use the get/set methods instead of directly accessing the structure.}
 */
typedef const struct { void *l; } * VuoRenderedLayers;

VuoRenderedLayers VuoRenderedLayers_makeEmpty(void);

void VuoRenderedLayers_setInteractions(VuoRenderedLayers renderedLayers, VuoList_VuoInteraction interactions);
void VuoRenderedLayers_setRootSceneObject(VuoRenderedLayers renderedLayers, VuoSceneObject rootSceneObject);
void VuoRenderedLayers_setWindow(VuoRenderedLayers renderedLayers, VuoWindowReference window);

VuoList_VuoInteraction VuoRenderedLayers_getInteractions(const VuoRenderedLayers renderedLayers);
VuoSceneObject VuoRenderedLayers_getRootSceneObject(const VuoRenderedLayers renderedLayers);
bool VuoRenderedLayers_getRenderingDimensions(const VuoRenderedLayers renderedLayers, unsigned long int *pixelsWide, unsigned long int *pixelsHigh, float *backingScaleFactor) VuoWarnUnusedResult;
bool VuoRenderedLayers_getWindow(const VuoRenderedLayers renderedLayers, VuoWindowReference *window) VuoWarnUnusedResult;

bool VuoRenderedLayers_windowChanged(const VuoRenderedLayers accumulatedRenderedLayers, const VuoRenderedLayers newerRenderedLayers);
void VuoRenderedLayers_update(VuoRenderedLayers accumulatedRenderedLayers, const VuoRenderedLayers newerRenderedLayers, bool *renderingDimensionsChanged);

VuoRenderedLayers VuoRenderedLayers_make(VuoSceneObject rootSceneObject,
										 unsigned long int pixelsWide, unsigned long int pixelsHigh,
										 float backingScaleFactor,
										 VuoList_VuoInteraction interactions);

VuoRenderedLayers VuoRenderedLayers_makeWithWindow(VuoSceneObject rootSceneObject,
												   unsigned long int pixelsWide, unsigned long int pixelsHigh,
												   float backingScaleFactor,
												   VuoWindowReference window,
												   VuoList_VuoInteraction interactions);

bool VuoRenderedLayers_findLayer(VuoRenderedLayers renderedLayers, VuoText layerName,
								 VuoList_VuoSceneObject ancestorObjects, VuoSceneObject *foundObject) VuoWarnUnusedResult;
bool VuoRenderedLayers_findLayerId(VuoRenderedLayers renderedLayers, uint64_t layerId,
	VuoList_VuoSceneObject ancestorObjects, VuoSceneObject *foundObject) VuoWarnUnusedResult;

bool VuoRenderedLayers_getTransformedLayer(VuoRenderedLayers renderedLayers,
										   VuoList_VuoSceneObject ancestorObjects, VuoSceneObject targetObject,
										   VuoPoint2d *layerCenter, VuoPoint2d layerCorners[4],
										   bool includeChildrenInBounds) VuoWarnUnusedResult;

bool VuoRenderedLayers_getTransformedPoint(	VuoRenderedLayers renderedLayers,
											VuoList_VuoSceneObject ancestorObjects,
											VuoSceneObject targetObject,
											VuoPoint2d point,
											VuoPoint2d *transformedPoint) VuoWarnUnusedResult;

bool VuoRenderedLayers_getInverseTransformedPoint(  VuoRenderedLayers renderedLayers,
													VuoList_VuoSceneObject ancestorObjects,
													VuoSceneObject targetObject,
													VuoPoint2d point,
													VuoPoint2d *inverseTransformedPoint) VuoWarnUnusedResult;

bool VuoRenderedLayers_getInverseTransformedPointLayer( VuoRenderedLayers renderedLayers,
														uint64_t targetLayer,
														VuoPoint2d point,
														VuoPoint2d* localPoint) VuoWarnUnusedResult;

bool VuoRenderedLayers_getRect(VuoRenderedLayers renderedLayers, VuoSceneObject layer, VuoRectangle *rect) VuoWarnUnusedResult;

VuoPoint2d VuoRenderedLayers_getTextSize(VuoRenderedLayers renderedLayers, VuoText text, VuoFont font, bool scaleWithScene, float verticalScale, float rotationZ, float wrapWidth, bool includeTrailingWhiteSpace);

VuoRectangle VuoRenderedLayers_getBoundingBox(VuoPoint2d layerCorners[4]);

bool VuoRenderedLayers_isPointInQuad(VuoPoint2d corners[4], VuoPoint2d point);

bool VuoRenderedLayers_isPointInLayer(VuoRenderedLayers renderedLayers, VuoText layerName, VuoPoint2d point);
bool VuoRenderedLayers_isPointInLayerId(VuoRenderedLayers renderedLayers, uint64_t layerId, VuoPoint2d point);

void VuoRenderedLayers_getEventsInLayer(VuoRenderedLayers renderedLayers,
										uint64_t id,
										bool *anyHover,
										bool *anyPressed,
										bool *anyReleased,
										bool *anyClicked);

VuoRenderedLayers VuoRenderedLayers_makeFromJson(struct json_object * js);
struct json_object * VuoRenderedLayers_getJson(const VuoRenderedLayers value);
char * VuoRenderedLayers_getSummary(const VuoRenderedLayers value);

///@{
/**
 * Automatically generated function.
 */
VuoRenderedLayers VuoRenderedLayers_makeFromString(const char *str);
char * VuoRenderedLayers_getString(const VuoRenderedLayers value);
void VuoRenderedLayers_retain(VuoRenderedLayers value);
void VuoRenderedLayers_release(VuoRenderedLayers value);
///@}

/**
 * @}
 */
