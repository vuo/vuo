/**
 * @file
 * VuoCubemap C type definition.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoImage.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoCubemap VuoCubemap
 * A collection of images for the 6 sides of a cube.
 * @{
 */

/**
 * A collection of images for the 6 sides of a cube.
 *
 * @version200New
 */
typedef const struct { void *l; } * VuoCubemap;

VuoCubemap VuoCubemap_makeFromImages(VuoImage front, VuoImage left, VuoImage right, VuoImage back, VuoImage top, VuoImage bottom);

VuoImage VuoCubemap_getFront(VuoCubemap cubemap);
VuoImage VuoCubemap_getLeft(VuoCubemap cubemap);
VuoImage VuoCubemap_getRight(VuoCubemap cubemap);
VuoImage VuoCubemap_getBack(VuoCubemap cubemap);
VuoImage VuoCubemap_getTop(VuoCubemap cubemap);
VuoImage VuoCubemap_getBottom(VuoCubemap cubemap);

VuoCubemap VuoCubemap_makeFromJson(struct json_object *js);
struct json_object * VuoCubemap_getJson(const VuoCubemap value);
char * VuoCubemap_getSummary(const VuoCubemap value);

///@{
/**
 * Automatically generated function.
 */
VuoCubemap VuoCubemap_makeFromString(const char *str);
char * VuoCubemap_getString(const VuoCubemap value);
void VuoCubemap_retain(VuoCubemap value);
void VuoCubemap_release(VuoCubemap value);
///@}

/**
 * @}
 */
