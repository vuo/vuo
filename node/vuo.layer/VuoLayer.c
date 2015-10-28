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
						"VuoColor",
						"VuoPoint2d",
						"VuoSceneObject",
						"VuoTransform2d",
						"VuoWindowReference",
						"VuoList_VuoColor",
						"VuoList_VuoLayer",
						"VuoList_VuoSceneObject"
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
 * Creates a visible layer that shows an image.
 *
 * @param name The layer's name (used by, e.g., @ref VuoRenderedLayers_findLayer).
 * @param image The image shown on the layer.
 * @param center The center of the layer, in Vuo Coordinates.
 * @param rotation The layer's angle, in degrees.
 * @param width The width of the layer, in Vuo Coordinates.
 * @param alpha The opacity of the layer, 0–1.
 */
VuoLayer VuoLayer_make(VuoText name, VuoImage image, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal alpha)
{
	VuoLayer o;
	VuoPoint3d center3d = VuoPoint3d_make(center.x, center.y, 0);
	VuoPoint3d rotation3d = VuoPoint3d_make(0, 0, rotation);
	o.sceneObject = VuoSceneObject_makeImage(image, center3d, rotation3d, width, alpha);
	o.sceneObject.name = name;
	return o;
}

/**
 * Creates a visible layer that shows an image.
 * The layer is the exact pixel-perfect size of the image (regardless of transform),
 * and the position is quantized so edges land on whole pixels.
 *
 * @param name The layer's name (used by, e.g., @ref VuoRenderedLayers_findLayer).
 * @param image The image shown on the layer.
 * @param center The center of the layer, in Vuo Coordinates.
 * @param alpha The opacity of the layer, 0–1.
 */
VuoLayer VuoLayer_makeRealSize(VuoText name, VuoImage image, VuoPoint2d center, VuoReal alpha)
{
	VuoLayer l = VuoLayer_make(name,image,center,0,0,alpha);
	l.sceneObject.isRealSize = true;
	return l;
}

/**
 * Creates a layer with a shadow, optionally real size.
 *
 * @param name The layer's name (used by, e.g., @ref VuoRenderedLayers_findLayer).
 * @param image The image shown on the layer.
 * @param center The center of the layer, in Vuo Coordinates.
 * @param rotation The layer's angle, in degrees.
 * @param width The width of the layer, in Vuo Coordinates.
 * @param alpha The opacity of the layer, 0–1.
 * @param shadowColor The color of the layer's shadow.
 * @param shadowBlur The amount to blur the layer's shadow, in pixels.
 * @param shadowAngle The angle of the layer's shadow, in degrees.
 * @param shadowDistance The distance that the shadow is offset from the layer, in Vuo Coordinates.
 * @param isRealSize Should this layer be rendered at actual (pixel-perfect) size?
 */
static VuoLayer VuoLayer_makeWithShadowInternal(VuoText name, VuoImage image, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal alpha, VuoColor shadowColor, VuoReal shadowBlur, VuoReal shadowAngle, VuoReal shadowDistance, VuoBoolean isRealSize)
{
	if (!image)
	{
		VuoLayer l;
		l.sceneObject = VuoSceneObject_makeEmpty();
		return l;
	}

	// Create a pair of layers, one for the main layer and one for the shadow, and put them in a group.
	// Apply the transformation to the individual layers, rather than to the group, so that
	// VuoRenderedLayers_getTransformedLayer() can work with the individual layers.

	float rotationInRadians = rotation * M_PI/180.;
	VuoTransform groupTransform = VuoTransform_makeFrom2d( VuoTransform2d_make(center, rotationInRadians, VuoPoint2d_make(width,width)) );
	float matrix[16];
	VuoTransform_getMatrix(groupTransform, matrix);

	VuoPoint3d center3d = VuoPoint3d_make(0,0,0);
	VuoPoint3d layerCenter3d = VuoTransform_transformPoint(matrix, center3d);
	VuoPoint2d layerCenter = VuoPoint2d_make(layerCenter3d.x, layerCenter3d.y);
	VuoLayer layer = VuoLayer_make(name, image, layerCenter, rotation, width, alpha);
	layer.sceneObject.isRealSize = isRealSize;

	VuoList_VuoColor colors = VuoListCreate_VuoColor();
	VuoRetain(colors);
	VuoListAppendValue_VuoColor(colors, shadowColor);
	VuoImage recoloredImage = VuoImage_mapColors(image, colors, 1);
	VuoRetain(recoloredImage);
	VuoRelease(colors);

	VuoImage blurredImage = VuoImage_blur(recoloredImage, shadowBlur, TRUE);
	float shadowAngleInRadians = shadowAngle * M_PI/180.;
	VuoPoint3d shadowOffset3d = VuoPoint3d_make(shadowDistance * cos(shadowAngleInRadians),
												shadowDistance * sin(shadowAngleInRadians),
												0);
	VuoPoint3d shadowCenter3d = VuoTransform_transformPoint(matrix, shadowOffset3d);
	VuoPoint2d shadowCenter = VuoPoint2d_make(shadowCenter3d.x, shadowCenter3d.y);
	VuoReal shadowWidth = width * blurredImage->pixelsWide/image->pixelsWide;
	VuoLayer shadow = VuoLayer_make(NULL, blurredImage, shadowCenter, rotation, shadowWidth, alpha);
	shadow.sceneObject.isRealSize = isRealSize;

	VuoList_VuoLayer layers = VuoListCreate_VuoLayer();
	VuoRetain(layers);
	VuoListAppendValue_VuoLayer(layers, shadow);
	VuoListAppendValue_VuoLayer(layers, layer);

	VuoRelease(recoloredImage);

	VuoLayer group = VuoLayer_makeGroup(layers, VuoTransform2d_makeIdentity());
	VuoRelease(layers);

	return group;
}

/**
 * Creates a visible layer with a shadow.
 *
 * @param name The layer's name (used by, e.g., @ref VuoRenderedLayers_findLayer).
 * @param image The image shown on the layer.
 * @param center The center of the layer, in Vuo Coordinates.
 * @param rotation The layer's angle, in degrees.
 * @param width The width of the layer, in Vuo Coordinates.
 * @param alpha The opacity of the layer, 0–1.
 * @param shadowColor The color of the layer's shadow.
 * @param shadowBlur The amount to blur the layer's shadow, in pixels.
 * @param shadowAngle The angle of the layer's shadow, in degrees.
 * @param shadowDistance The distance that the shadow is offset from the layer, in Vuo Coordinates.
 */
VuoLayer VuoLayer_makeWithShadow(VuoText name, VuoImage image, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal alpha, VuoColor shadowColor, VuoReal shadowBlur, VuoReal shadowAngle, VuoReal shadowDistance)
{
	return VuoLayer_makeWithShadowInternal(name,image,center,rotation,width,alpha,shadowColor,shadowBlur,shadowAngle,shadowDistance,false);
}

/**
 * Creates a visible layer with a shadow.
 * The layer is the exact pixel-perfect size of the image (regardless of transform),
 * and the position is quantized so edges land on whole pixels.
 *
 * @param name The layer's name (used by, e.g., @ref VuoRenderedLayers_findLayer).
 * @param image The image shown on the layer.
 * @param center The center of the layer, in Vuo Coordinates.
 * @param alpha The opacity of the layer, 0–1.
 * @param shadowColor The color of the layer's shadow.
 * @param shadowBlur The amount to blur the layer's shadow, in pixels.
 * @param shadowAngle The angle of the layer's shadow, in degrees.
 * @param shadowDistance The distance that the shadow is offset from the layer, in Vuo Coordinates.
 */
VuoLayer VuoLayer_makeRealSizeWithShadow(VuoText name, VuoImage image, VuoPoint2d center, VuoReal alpha, VuoColor shadowColor, VuoReal shadowBlur, VuoReal shadowAngle, VuoReal shadowDistance)
{
	return VuoLayer_makeWithShadowInternal(name,image,center,0,1,alpha,shadowColor,shadowBlur,shadowAngle,shadowDistance,true);
}

/**
 * Creates a visible layer with the specified color
 *
 * @param name The layer's name (used by, e.g., @ref VuoRenderedLayers_findLayer).
 * @param color The layer's color.
 * @param center The center of the layer, in Vuo Coordinates.
 * @param rotation The layer's angle, in degrees.
 * @param width The width of the layer, in Vuo Coordinates.
 * @param height The height of the layer, in Vuo Coordinates.
 */
VuoLayer VuoLayer_makeColor(VuoText name, VuoColor color, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height)
{
	VuoLayer o;
	o.sceneObject = VuoSceneObject_makeQuad(
				VuoShader_makeUnlitColorShader(color),
				VuoPoint3d_make(center.x, center.y, 0),
				VuoPoint3d_make(0, 0, rotation),
				width,
				height
			);
	o.sceneObject.name = name;
	return o;
}

/**
 * Creates a visible layer with a linear gradient.
 *
 * @param name The layer's name (used by, e.g., @ref VuoRenderedLayers_findLayer).
 * @param colors The layer's gradient colors.
 * @param start The start point of the gradient, in Vuo Coordinates.
 * @param end The end point of the gradient, in Vuo Coordinates.
 * @param center The center of the layer, in Vuo Coordinates.
 * @param rotation The layer's angle, in degrees.
 * @param width The width of the layer, in Vuo Coordinates.
 * @param height The height of the layer, in Vuo Coordinates.
 */
VuoLayer VuoLayer_makeLinearGradient(VuoText name, VuoList_VuoColor colors, VuoPoint2d start, VuoPoint2d end, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height)
{
	VuoLayer o;
	o.sceneObject = VuoSceneObject_makeQuad(
				VuoShader_makeLinearGradientShader(colors, start, end),
				VuoPoint3d_make(center.x, center.y, 0),
				VuoPoint3d_make(0, 0, rotation),
				width,
				height
			);
	o.sceneObject.name = name;
	return o;
}

/**
 * Creates a visible layer with a radial gradient.
 *
 * @param name The layer's name (used by, e.g., @ref VuoRenderedLayers_findLayer).
 * @param colors The layer's gradient colors.
 * @param gradientCenter The center point of the gradient, in Vuo Coordinates.
 * @param radius The radius of the gradient, in Vuo Coordinates.
 * @param center The center of the layer, in Vuo Coordinates.
 * @param rotation The layer's angle, in degrees.
 * @param width The width of the layer, in Vuo Coordinates.
 * @param height The height of the layer, in Vuo Coordinates.
 */
VuoLayer VuoLayer_makeRadialGradient(VuoText name, VuoList_VuoColor colors, VuoPoint2d gradientCenter, VuoReal radius, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height)
{
	VuoLayer o;
	o.sceneObject = VuoSceneObject_makeQuad(
				VuoShader_makeRadialGradientShader(colors, gradientCenter, radius, width, height),
				VuoPoint3d_make(center.x, center.y, 0),
				VuoPoint3d_make(0, 0, rotation),
				width,
				height
			);
	o.sceneObject.name = name;
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
		VuoListAppendValue_VuoSceneObject(o.sceneObject.childObjects, VuoListGetValue_VuoLayer(childLayers, i).sceneObject);

	return o;
}

/**
 * Returns a rectangle enclosing the sceneobject (which is assumed to be 2-dimensional).
 */
static VuoRectangle VuoLayer_getBoundingRectangleWithSceneObject(VuoSceneObject so, VuoInteger viewportWidth, VuoInteger viewportHeight)
{
	VuoRectangle b = VuoRectangle_make(0,0,0,0);

	float matrix[16];

	if (so.shader)
	{
		VuoImage image = VuoShader_getUniform_VuoImage(so.shader, "texture");
		if (image && so.isRealSize)
			VuoTransform_getBillboardMatrix(image->pixelsWide, image->pixelsHigh, so.transform.translation.x, so.transform.translation.y, viewportWidth, viewportHeight, matrix);
		else
			VuoTransform_getMatrix(so.transform, matrix);

		b = VuoTransform_transformRectangle(matrix, VuoRectangle_make(0,0,1,1));
	}
	else
		VuoTransform_getMatrix(so.transform, matrix);

	if (so.childObjects)
	{
		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject child = VuoListGetValue_VuoSceneObject(so.childObjects, i);
			VuoRectangle childBoundingBox = VuoLayer_getBoundingRectangleWithSceneObject(child, viewportWidth, viewportHeight);
			childBoundingBox = VuoTransform_transformRectangle(matrix, childBoundingBox);
			b = VuoPoint2d_rectangleUnion(b, childBoundingBox);
		}
	}

	return b;
}

/**
 * Returns the minimal rectangle enclosing the layer and its child layers.
 */
VuoRectangle VuoLayer_getBoundingRectangle(VuoLayer layer, VuoInteger viewportWidth, VuoInteger viewportHeight)
{
	return VuoLayer_getBoundingRectangleWithSceneObject(layer.sceneObject, viewportWidth, viewportHeight);
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
