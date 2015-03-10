/**
 * @file
 * VuoLayer implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoLayer.h"
#include "VuoList_VuoSceneObject.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Layer",
					 "description" : "A 2D Layer: visible (image), or virtual (group).",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoSceneObject",
						 "c",
						 "json"
					 ]
				 });
#endif
/// @}


/**
 * Creates a new, empty scene object.
 */
VuoLayer VuoLayer_makeEmpty(void)
{
	VuoLayer o;
	o.sceneObject = VuoSceneObject_makeEmpty();
	return o;
}

/**
 * Creates a visible (image) layer.
 */
VuoLayer VuoLayer_make(VuoImage image, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal alpha)
{
	VuoLayer o;
	VuoPoint3d center3d = VuoPoint3d_make(center.x, center.y, 0);
	VuoPoint3d rotation3d = VuoPoint3d_make(0, 0, rotation);
	o.sceneObject = VuoSceneObject_makeImage(image, center3d, rotation3d, width, alpha);
	return o;
}

/**
 * Creates a visible (image) layer.
 * The layer is the exact pixel-perfect size of the image (regardless of transform),
 * and the position is quantized so edges land on whole pixels.
 */
VuoLayer VuoLayer_makeRealSize(VuoImage image, VuoPoint2d center, VuoReal alpha)
{
	VuoLayer l = VuoLayer_make(image,center,0,0,alpha);
	l.sceneObject.isRealSize = true;
	return l;
}

/**
 * Creates a visible layer with the specified color
 */
VuoLayer VuoLayer_makeColor(VuoColor color, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height)
{
	VuoLayer o;
	o.sceneObject = VuoSceneObject_makeQuad(
				VuoShader_makeColorShader(color),
				VuoPoint3d_make(center.x, center.y, 0),
				VuoPoint3d_make(0, 0, rotation),
				width,
				height
			);
	return o;
}

/**
 * Creates a visible layer with a gradient.
 */
VuoLayer VuoLayer_makeLinearGradient(VuoList_VuoColor colors, VuoPoint2d start, VuoPoint2d end, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height)
{
	VuoLayer o;
	o.sceneObject = VuoSceneObject_makeQuad(
				VuoShader_makeLinearGradientShader(colors, start, end),
				VuoPoint3d_make(center.x, center.y, 0),
				VuoPoint3d_make(0, 0, rotation),
				width,
				height
			);
	return o;
}

/**
 * Creates a visible layer with a gradient.
 */
VuoLayer VuoLayer_makeRadialGradient(VuoList_VuoColor colors, VuoPoint2d gradientCenter, VuoReal radius, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height)
{
	VuoLayer o;
	o.sceneObject = VuoSceneObject_makeQuad(
				VuoShader_makeRadialGradientShader(colors, gradientCenter, radius, width, height),
				VuoPoint3d_make(center.x, center.y, 0),
				VuoPoint3d_make(0, 0, rotation),
				width,
				height
			);
	return o;
}

/**
 * Creates a layer with a group of child layers.
 */
VuoLayer VuoLayer_makeGroup(VuoList_VuoLayer childLayers, VuoTransform2d transform)
{
	VuoLayer o;
	o.sceneObject = VuoSceneObject_makeEmpty();
	o.sceneObject.transform = VuoTransform_makeFrom2d(transform);
	o.sceneObject.childObjects = VuoListCreate_VuoSceneObject();

	unsigned long childLayerCount = VuoListGetCount_VuoLayer(childLayers);
	for (unsigned long i = 1; i <= childLayerCount; ++i)
		VuoListAppendValue_VuoSceneObject(o.sceneObject.childObjects, VuoListGetValueAtIndex_VuoLayer(childLayers, i).sceneObject);

	return o;
}

/**
 * @ingroup VuoLayer
 * @see VuoSceneObject_valueFromJson
 */
VuoLayer VuoLayer_valueFromJson(json_object * js)
{
	VuoLayer o;
	o.sceneObject = VuoSceneObject_valueFromJson(js);
	return o;
}

/**
 * @ingroup VuoLayer
 * @see VuoSceneObject_jsonFromValue
 */
json_object * VuoLayer_jsonFromValue(const VuoLayer value)
{
	return VuoSceneObject_jsonFromValue(value.sceneObject);
}

/**
 * @ingroup VuoLayer
 * @see VuoSceneObject_summaryFromValue
 */
char * VuoLayer_summaryFromValue(const VuoLayer value)
{
	return VuoSceneObject_summaryFromValue(value.sceneObject);
}
