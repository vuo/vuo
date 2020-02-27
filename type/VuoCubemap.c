/**
 * @file
 * VuoCubemap implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include "VuoCubemap.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "Cubemap",
	"description" : "A collection of images for the 6 sides of a cube",
	"keywords" : [
		"skybox", "environment map", "360", "180", "VR", "360vr",
	],
	"version" : "1.0.0",
	"dependencies" : [
		"VuoImage",
		"VuoText",
	],
});
#endif
/// @}

/**
 * @private VuoCubemap fields.
 */
typedef struct
{
	VuoImage front;
	VuoImage left;
	VuoImage right;
	VuoImage back;
	VuoImage top;
	VuoImage bottom;
} VuoCubemap_internal;

/**
 * @private Frees the memory associated with the object.
 *
 * @threadAny
 */
void VuoCubemap_free(void *cubemap)
{
	VuoCubemap_internal *c = (VuoCubemap_internal *)cubemap;

	VuoRelease(c->front);
	VuoRelease(c->left);
	VuoRelease(c->right);
	VuoRelease(c->back);
	VuoRelease(c->top);
	VuoRelease(c->bottom);

	free(c);
}

/**
 * Creates a cubemap consisting of images for the 6 sides of a cube.
 *
 * @version200New
 */
VuoCubemap VuoCubemap_makeFromImages(VuoImage front, VuoImage left, VuoImage right, VuoImage back, VuoImage top, VuoImage bottom)
{
	VuoCubemap_internal *c = (VuoCubemap_internal *)calloc(1, sizeof(VuoCubemap_internal));
	VuoRegister(c, VuoCubemap_free);

	VuoRetain(front);
	c->front = front;

	VuoRetain(left);
	c->left = left;

	VuoRetain(right);
	c->right = right;

	VuoRetain(back);
	c->back = back;

	VuoRetain(top);
	c->top = top;

	VuoRetain(bottom);
	c->bottom = bottom;

	return (VuoCubemap)c;
}

/**
 * Returns the image for the front side of the cubemap.
 *
 * @version200New
 */
VuoImage VuoCubemap_getFront(VuoCubemap cubemap)
{
	VuoCubemap_internal *c = (VuoCubemap_internal *)cubemap;
	return c->front;
}

/**
 * Returns the image for the left side of the cubemap.
 *
 * @version200New
 */
VuoImage VuoCubemap_getLeft(VuoCubemap cubemap)
{
	VuoCubemap_internal *c = (VuoCubemap_internal *)cubemap;
	return c->left;
}

/**
 * Returns the image for the right side of the cubemap.
 *
 * @version200New
 */
VuoImage VuoCubemap_getRight(VuoCubemap cubemap)
{
	VuoCubemap_internal *c = (VuoCubemap_internal *)cubemap;
	return c->right;
}

/**
 * Returns the image for the back of the cubemap.
 *
 * @version200New
 */
VuoImage VuoCubemap_getBack(VuoCubemap cubemap)
{
	VuoCubemap_internal *c = (VuoCubemap_internal *)cubemap;
	return c->back;
}

/**
 * Returns the image for the top of the cubemap.
 *
 * @version200New
 */
VuoImage VuoCubemap_getTop(VuoCubemap cubemap)
{
	VuoCubemap_internal *c = (VuoCubemap_internal *)cubemap;
	return c->top;
}

/**
 * Returns the image for the bottom of the cubemap.
 *
 * @version200New
 */
VuoImage VuoCubemap_getBottom(VuoCubemap cubemap)
{
	VuoCubemap_internal *c = (VuoCubemap_internal *)cubemap;
	return c->bottom;
}

/**
 * @ingroup VuoCubemap
 * @see VuoSceneObject_makeFromJson
 */
VuoCubemap VuoCubemap_makeFromJson(json_object *js)
{
	return VuoCubemap_makeFromImages(
		VuoJson_getObjectValue(VuoImage, js, "front", NULL),
		VuoJson_getObjectValue(VuoImage, js, "left", NULL),
		VuoJson_getObjectValue(VuoImage, js, "right", NULL),
		VuoJson_getObjectValue(VuoImage, js, "back", NULL),
		VuoJson_getObjectValue(VuoImage, js, "top", NULL),
		VuoJson_getObjectValue(VuoImage, js, "bottom", NULL));
}

/**
 * @ingroup VuoCubemap
 * @see VuoSceneObject_getJson
 */
json_object * VuoCubemap_getJson(const VuoCubemap cubemap)
{
	VuoCubemap_internal *c = (VuoCubemap_internal *)cubemap;

	json_object *js = json_object_new_object();
	json_object_object_add(js, "front",  VuoImage_getJson(c->front));
	json_object_object_add(js, "left",   VuoImage_getJson(c->left));
	json_object_object_add(js, "right",  VuoImage_getJson(c->right));
	json_object_object_add(js, "back",   VuoImage_getJson(c->back));
	json_object_object_add(js, "top",    VuoImage_getJson(c->top));
	json_object_object_add(js, "bottom", VuoImage_getJson(c->bottom));

	return js;
}

/**
 * @ingroup VuoCubemap
 * @see VuoSceneObject_getSummary
 */
char *VuoCubemap_getSummary(const VuoCubemap cubemap)
{
	VuoCubemap_internal *c = (VuoCubemap_internal *)cubemap;

	return VuoText_format("<table><tr><th>Front</th><td>%lu×%lu</td></tr><tr><th>Left</th><td>%lu×%lu</td></tr><tr><th>Right</th><td>%lu×%lu</td></tr><tr><th>Back</th><td>%lu×%lu</td></tr><tr><th>Top</th><td>%lu×%lu</td></tr><tr><th>Bottom</th><td>%lu×%lu</td></tr>",
		c->front  ? c->front->pixelsWide  : 0, c->front  ? c->front->pixelsHigh  : 0,
		c->left   ? c->left->pixelsWide   : 0, c->left   ? c->left->pixelsHigh   : 0,
		c->right  ? c->right->pixelsWide  : 0, c->right  ? c->right->pixelsHigh  : 0,
		c->back   ? c->back->pixelsWide   : 0, c->back   ? c->back->pixelsHigh   : 0,
		c->top    ? c->top->pixelsWide    : 0, c->top    ? c->top->pixelsHigh    : 0,
		c->bottom ? c->bottom->pixelsWide : 0, c->bottom ? c->bottom->pixelsHigh : 0);
}
