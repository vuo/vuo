/**
 * @file
 * VuoLayer implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoLayer.h"
#include "VuoList_VuoSceneObject.h"
#include "VuoImageBlur.h"
#include "VuoImageText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Layer",
					 "description" : "A 2D Layer: visible (image), or virtual (group).",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoColor",
						"VuoImageBlur",
						"VuoImageText",
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
 *
 * @param name The layer's name (used by, e.g., @ref VuoRenderedLayers_findLayer).
 * @param image The image shown on the layer.
 * @param transform The 2d transform that specifies translation, rotation, and scale relative to this layer's parent.
 * @param alpha The opacity of the layer, 0–1.
 */
VuoLayer VuoLayer_makeWithTransform(VuoText name, VuoImage image, VuoTransform2d transform, VuoReal alpha)
{
	VuoLayer o;
	VuoPoint3d center3d = VuoPoint3d_make(transform.translation.x, transform.translation.y, 0);
	// VuoSceneObject_makeImage wants rotation in degrees
	VuoPoint3d rotation3d = VuoPoint3d_make(0, 0, transform.rotation * 57.295779513f);
	o.sceneObject = VuoSceneObject_makeImage(image, center3d, rotation3d, 2, alpha);
	o.sceneObject.transform.scale = VuoPoint3d_make(transform.scale.x, transform.scale.y, 1);
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
 * @param preservePhysicalSize See @ref VuoSceneObject::preservePhysicalSize
 */
VuoLayer VuoLayer_makeRealSize(VuoText name, VuoImage image, VuoPoint2d center, VuoReal alpha, VuoBoolean preservePhysicalSize)
{
	VuoLayer l = VuoLayer_make(name,image,center,0,0,alpha);
	l.sceneObject.isRealSize = true;
	l.sceneObject.preservePhysicalSize = preservePhysicalSize;
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
 * @param preservePhysicalSize See @ref VuoSceneObject::preservePhysicalSize
 * @param shadowColor The color of the layer's shadow.
 * @param shadowBlur The amount to blur the layer's shadow, in pixels.
 * @param shadowAngle The angle of the layer's shadow, in degrees.
 * @param shadowDistance The distance that the shadow is offset from the layer, in Vuo Coordinates.
 * @param isRealSize Should this layer be rendered at actual (pixel-perfect) size?
 */
static VuoLayer VuoLayer_makeWithShadowInternal(VuoText name, VuoImage image, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal alpha, VuoBoolean preservePhysicalSize, VuoColor shadowColor, VuoReal shadowBlur, VuoReal shadowAngle, VuoReal shadowDistance, VuoBoolean isRealSize)
{
	if (!image)
		return VuoLayer_makeEmpty();

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
	layer.sceneObject.preservePhysicalSize = preservePhysicalSize;

	VuoList_VuoColor colors = VuoListCreate_VuoColor();
	VuoRetain(colors);
	VuoListAppendValue_VuoColor(colors, shadowColor);
	VuoImage recoloredImage = VuoImage_mapColors(image, colors, 1);
	VuoRetain(recoloredImage);
	VuoRelease(colors);

	VuoImageBlur *ib = VuoImageBlur_make();
	VuoRetain(ib);
	VuoImage blurredImage = VuoImageBlur_blur(ib, recoloredImage, NULL, VuoBlurShape_Gaussian, shadowBlur, 1, TRUE);
	VuoRelease(ib);
	float shadowAngleInRadians = shadowAngle * M_PI/180.;
	VuoPoint3d shadowOffset3d = VuoPoint3d_make(shadowDistance * cos(shadowAngleInRadians),
												shadowDistance * sin(shadowAngleInRadians),
												0);
	VuoPoint3d shadowCenter3d = VuoTransform_transformPoint(matrix, shadowOffset3d);
	VuoPoint2d shadowCenter = VuoPoint2d_make(shadowCenter3d.x, shadowCenter3d.y);
	VuoReal shadowWidth = width * blurredImage->pixelsWide/image->pixelsWide;
	VuoLayer shadow = VuoLayer_make(NULL, blurredImage, shadowCenter, rotation, shadowWidth, alpha);
	shadow.sceneObject.isRealSize = isRealSize;
	shadow.sceneObject.preservePhysicalSize = preservePhysicalSize;

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
	return VuoLayer_makeWithShadowInternal(name,image,center,rotation,width,alpha,false,shadowColor,shadowBlur,shadowAngle,shadowDistance,false);
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
 * @param preservePhysicalSize See @ref VuoSceneObject::preservePhysicalSize
 * @param shadowColor The color of the layer's shadow.
 * @param shadowBlur The amount to blur the layer's shadow, in pixels.
 * @param shadowAngle The angle of the layer's shadow, in degrees.
 * @param shadowDistance The distance that the shadow is offset from the layer, in Vuo Coordinates.
 */
VuoLayer VuoLayer_makeRealSizeWithShadow(VuoText name, VuoImage image, VuoPoint2d center, VuoReal alpha, VuoBoolean preservePhysicalSize, VuoColor shadowColor, VuoReal shadowBlur, VuoReal shadowAngle, VuoReal shadowDistance)
{
	return VuoLayer_makeWithShadowInternal(name,image,center,0,1,alpha,preservePhysicalSize,shadowColor,shadowBlur,shadowAngle,shadowDistance,true);
}

/**
 * Creates a rectangular layer with the specified color
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
 * Creates an oval layer with the specified color
 *
 * @param name The layer's name (used by, e.g., @ref VuoRenderedLayers_findLayer).
 * @param color The layer's color.
 * @param center The center of the layer, in Vuo Coordinates.
 * @param rotation The layer's angle, in degrees.
 * @param width The width of the layer, in Vuo Coordinates.
 * @param height The height of the layer, in Vuo Coordinates.
 * @param sharpness How sharp the edge of the oval is, from 0 (blurry) to 1 (sharp).
 */
VuoLayer VuoLayer_makeOval(VuoText name, VuoColor color, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height, VuoReal sharpness)
{
	VuoLayer o;
	// Since VuoShader_makeUnlitCircleShader() produces a shader that fills half the size (to leave enough room for sharpness=0),
	// make the layer twice the specified size.
	o.sceneObject = VuoSceneObject_makeQuad(
				VuoShader_makeUnlitCircleShader(color, sharpness),
				VuoPoint3d_make(center.x, center.y, 0),
				VuoPoint3d_make(0, 0, rotation),
				width*2,
				height*2
			);
	o.sceneObject.name = name;
	return o;
}

/**
 * Creates a rounded rectangle layer with the specified color
 *
 * @param name The layer's name (used by, e.g., @ref VuoRenderedLayers_findLayer).
 * @param color The layer's color.
 * @param center The center of the layer, in Vuo Coordinates.
 * @param rotation The layer's angle, in degrees.
 * @param width The width of the layer, in Vuo Coordinates.
 * @param height The height of the layer, in Vuo Coordinates.
 * @param sharpness How sharp the edges of the rectangle are, from 0 (blurry) to 1 (sharp).
 * @param roundness How round the corners of the rectangle are, from 0 (rectangular) to 1 (circular / capsular).
 */
VuoLayer VuoLayer_makeRoundedRectangle(VuoText name, VuoColor color, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height, VuoReal sharpness, VuoReal roundness)
{
	VuoLayer o;

	// Since VuoShader_makeUnlitRoundedRectangleShader() produces a shader that fills half the size (to leave enough room for sharpness=0),
	// make the layer twice the specified size.
	o.sceneObject = VuoSceneObject_makeQuad(
				VuoShader_makeUnlitRoundedRectangleShader(color, sharpness, roundness, width/height),
				VuoPoint3d_make(center.x, center.y, 0),
				VuoPoint3d_make(0, 0, rotation),
				width  * 2,
				height * 2
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
 * @param noiseAmount How much random noise to add to the gradient.  Typically between 0 and 1.
 */
VuoLayer VuoLayer_makeLinearGradient(VuoText name, VuoList_VuoColor colors, VuoPoint2d start, VuoPoint2d end, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height, VuoReal noiseAmount)
{
	VuoShader shader = VuoShader_makeLinearGradientShader();
	VuoShader_setLinearGradientShaderValues(shader, colors, start, end, 1, noiseAmount);
	VuoLayer o;
	o.sceneObject = VuoSceneObject_makeQuad(
				shader,
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
 * @param noiseAmount How much random noise to add to the gradient.  Typically between 0 and 1.
 */
VuoLayer VuoLayer_makeRadialGradient(VuoText name, VuoList_VuoColor colors, VuoPoint2d gradientCenter, VuoReal radius, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height, VuoReal noiseAmount)
{
	VuoShader shader = VuoShader_makeRadialGradientShader();
	VuoShader_setRadialGradientShaderValues(shader, colors, gradientCenter, radius, width, height, noiseAmount);
	VuoLayer o;
	o.sceneObject = VuoSceneObject_makeQuad(
				shader,
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
	o.sceneObject = VuoSceneObject_makeGroup(VuoListCreate_VuoSceneObject(), VuoTransform_makeFrom2d(transform));

	unsigned long childLayerCount = VuoListGetCount_VuoLayer(childLayers);
	for (unsigned long i = 1; i <= childLayerCount; ++i)
		VuoListAppendValue_VuoSceneObject(o.sceneObject.childObjects, VuoListGetValue_VuoLayer(childLayers, i).sceneObject);

	return o;
}

/**
 * Returns a list of this layer's child layers.
 */
VuoList_VuoLayer VuoLayer_getChildLayers(VuoLayer layer)
{
	unsigned long childLayerCount = VuoListGetCount_VuoSceneObject(layer.sceneObject.childObjects);
	if (childLayerCount == 0)
		return NULL;

	VuoList_VuoLayer childLayers = VuoListCreateWithCount_VuoLayer(childLayerCount, VuoLayer_makeEmpty());
	VuoLayer *childLayersData = VuoListGetData_VuoLayer(childLayers);
	VuoSceneObject *objects = VuoListGetData_VuoSceneObject(layer.sceneObject.childObjects);
	for (unsigned int i = 0; i < childLayerCount; ++i)
	{
		childLayersData[i] = (VuoLayer){objects[i]};
		VuoLayer_retain(childLayersData[i]);
	}
	return childLayers;
}

/**
 * Returns a rectangle enclosing the sceneobject (which is assumed to be 2-dimensional).
 */
static VuoRectangle VuoLayer_getBoundingRectangleWithSceneObject(VuoSceneObject so, VuoInteger viewportWidth, VuoInteger viewportHeight, float backingScaleFactor)
{
	VuoRectangle b = VuoRectangle_make(0,0,0,0);

	float matrix[16];

	if (so.type == VuoSceneObjectSubType_Text)
	{
		b = VuoImage_getTextRectangle(so.text, so.font, true);
		b.size = VuoPoint2d_multiply(b.size, backingScaleFactor * 2./(viewportWidth));
	}

	if (so.shader)
	{
		VuoImage image = VuoShader_getUniform_VuoImage(so.shader, "texture");
		if (image && so.isRealSize)
			VuoTransform_getBillboardMatrix(image->pixelsWide, image->pixelsHigh, image->scaleFactor, so.preservePhysicalSize, so.transform.translation.x, so.transform.translation.y, viewportWidth, viewportHeight, backingScaleFactor, so.mesh->submeshes[0].positions[0].x, matrix);
		else
			VuoTransform_getMatrix(so.transform, matrix);

		b = VuoTransform_transformRectangle(matrix, VuoRectangle_make(0,0,so.shader->objectScale,so.shader->objectScale));
	}
	else
		VuoTransform_getMatrix(so.transform, matrix);

	if (so.childObjects)
	{
		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject child = VuoListGetValue_VuoSceneObject(so.childObjects, i);
			VuoRectangle childBoundingBox = VuoLayer_getBoundingRectangleWithSceneObject(child, viewportWidth, viewportHeight, backingScaleFactor);
			childBoundingBox = VuoTransform_transformRectangle(matrix, childBoundingBox);
			b = VuoPoint2d_rectangleUnion(b, childBoundingBox);
		}
	}

	return b;
}

/**
 * Moves the pivot point of `child` by creating a parent layer and assigning child
 * with an offset.
 *
 * If anchor is { Center, Center } the unmodified layer is returned.
 */
VuoLayer VuoLayer_setAnchor(VuoLayer child, VuoAnchor anchor, VuoInteger viewportWidth, VuoInteger viewportHeight, float backingScaleFactor)
{
	if (VuoAnchor_areEqual(anchor, VuoAnchor_makeCentered()))
		return child;

	VuoTransform childTransform = child.sceneObject.transform;
	child.sceneObject.transform = VuoTransform_makeIdentity();

	VuoRectangle rect = VuoLayer_getBoundingRectangle(child, viewportWidth, viewportHeight, backingScaleFactor);

	VuoPoint3d childTranslation = childTransform.translation;
	VuoPoint2d boundsCenter = rect.center;
	VuoPoint3d parentTranslation = VuoPoint3d_make(childTranslation.x - boundsCenter.x, childTranslation.y - boundsCenter.y, 0);

	if (VuoAnchor_getHorizontal(anchor) == VuoHorizontalAlignment_Left)
		boundsCenter.x += rect.size.x * .5f;
	else if (VuoAnchor_getHorizontal(anchor) == VuoHorizontalAlignment_Right)
		boundsCenter.x -= rect.size.x * .5f;

	if (VuoAnchor_getVertical(anchor) == VuoVerticalAlignment_Top)
		boundsCenter.y -= rect.size.y * .5f;
	else if (VuoAnchor_getVertical(anchor) == VuoVerticalAlignment_Bottom)
		boundsCenter.y += rect.size.y * .5f;

	child.sceneObject.transform.translation = VuoPoint3d_make(boundsCenter.x, boundsCenter.y, 0);

	VuoTransform parentTransform = childTransform;
	parentTransform.translation = parentTranslation;

	VuoLayer parent;
	parent.sceneObject = VuoSceneObject_makeGroup(VuoListCreate_VuoSceneObject(), parentTransform);

	parent.sceneObject.name = child.sceneObject.name;
	child.sceneObject.name = VuoText_make(VuoText_format("%s (Child)", parent.sceneObject.name));

	VuoListAppendValue_VuoSceneObject(parent.sceneObject.childObjects, child.sceneObject);

	return parent;
}

/**
 * Returns the minimal rectangle enclosing the layer and its child layers.
 */
VuoRectangle VuoLayer_getBoundingRectangle(VuoLayer layer, VuoInteger viewportWidth, VuoInteger viewportHeight, float backingScaleFactor)
{
	return VuoLayer_getBoundingRectangleWithSceneObject(layer.sceneObject, viewportWidth, viewportHeight, backingScaleFactor);
}

/**
 * Returns true if the layer or any of its children have a non-empty type.
 */
bool VuoLayer_isPopulated(VuoLayer layer)
{
	return VuoSceneObject_isPopulated(layer.sceneObject);
}

/**
 * @ingroup VuoLayer
 * @see VuoSceneObject_makeFromJson
 */
VuoLayer VuoLayer_makeFromJson(json_object * js)
{
	VuoLayer o;
	o.sceneObject = VuoSceneObject_makeFromJson(js);
	return o;
}

/**
 * @ingroup VuoLayer
 * @see VuoSceneObject_getJson
 */
json_object * VuoLayer_getJson(const VuoLayer value)
{
	return VuoSceneObject_getJson(value.sceneObject);
}

/**
 * @ingroup VuoLayer
 * @see VuoSceneObject_getSummary
 */
char * VuoLayer_getSummary(const VuoLayer value)
{
	return VuoSceneObject_getSummary(value.sceneObject);
}
