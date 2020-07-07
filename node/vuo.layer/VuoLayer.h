/**
 * @file
 * VuoLayer C type definition.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoAnchor.h"
#include "VuoColor.h"
#include "VuoPoint2d.h"
#include "VuoSceneObject.h"
#include "VuoTransform2d.h"
#include "VuoWindowReference.h"
#include "VuoList_VuoColor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoLayer VuoLayer
 * A 2D Object: visible (image), or virtual (group).
 *
 * @{
 */

/**
 * A 2D Object: visible (image), or virtual (group).
 *
 * @version200Changed{VuoLayer is now an opaque, heap-allocated type.
 * Please use the get/set methods instead of directly accessing the structure.}
 */
typedef const struct { void *l; } * VuoLayer;

#include "VuoList_VuoLayer.h"

VuoLayer VuoLayer_makeEmpty(void);
VuoLayer VuoLayer_makeGroup(VuoList_VuoLayer childLayers, VuoTransform2d transform);
VuoLayer VuoLayer_makeGroup2(VuoLayer layer1, VuoLayer layer2, VuoTransform2d transform);
VuoLayer VuoLayer_makeGroup3(VuoLayer layer1, VuoLayer layer2, VuoLayer layer3, VuoTransform2d transform);
VuoList_VuoLayer VuoLayer_getChildLayers(VuoLayer layer);

VuoLayer VuoLayer_make(VuoText name, VuoImage image, VuoPoint2d center, VuoReal rotation, VuoReal size, VuoOrientation fixed, VuoReal alpha);
VuoLayer VuoLayer_makeWithTransform(VuoText name, VuoImage image, VuoTransform2d transform, VuoReal alpha);
VuoLayer VuoLayer_makeWithShadow(VuoText name, VuoImage image, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal alpha, VuoColor shadowColor, VuoReal shadowBlur, VuoReal shadowAngle, VuoReal shadowDistance);
VuoLayer VuoLayer_makeRealSize(VuoText name, VuoImage image, VuoPoint2d center, VuoReal alpha, VuoBoolean preservePhysicalSize);
VuoLayer VuoLayer_makeRealSizeWithShadow(VuoText name, VuoImage image, VuoPoint2d center, VuoReal alpha, VuoBoolean preservePhysicalSize, VuoColor shadowColor, VuoReal shadowBlur, VuoReal shadowAngle, VuoReal shadowDistance);

VuoLayer VuoLayer_makeColor(VuoText name, VuoColor color, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height);
VuoLayer VuoLayer_makeOval(VuoText name, VuoColor color, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height, VuoReal sharpness);
VuoLayer VuoLayer_makeRoundedRectangle(VuoText name, VuoColor color, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height, VuoReal sharpness, VuoReal roundness);
VuoLayer VuoLayer_makeCheckmark(VuoText name, VuoColor fillColor, VuoColor outlineColor, VuoReal outlineThickness, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height);

VuoLayer VuoLayer_makeLinearGradient(VuoText name, VuoList_VuoColor colors, VuoPoint2d start, VuoPoint2d end, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height, VuoReal noiseAmount);
VuoLayer VuoLayer_makeRadialGradient(VuoText name, VuoList_VuoColor colors, VuoPoint2d gradientCenter, VuoReal radius, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height, VuoReal noiseAmount);

uint64_t VuoLayer_getId(const VuoLayer layer);

void VuoLayer_setId(VuoLayer layer, uint64_t id);

VuoRectangle VuoLayer_getBoundingRectangle(VuoLayer layer, VuoInteger viewportWidth, VuoInteger viewportHeight, float backingScaleFactor);
VuoLayer VuoLayer_setAnchor(VuoLayer child, VuoAnchor anchor, VuoInteger viewportWidth, VuoInteger viewportHeight, float backingScaleFactor);

bool VuoLayer_isPopulated(VuoLayer layer);

VuoLayer VuoLayer_makeFromJson(struct json_object * js);
struct json_object * VuoLayer_getJson(const VuoLayer value);
char * VuoLayer_getSummary(const VuoLayer value);

///@{
/**
 * Automatically generated function.
 */
VuoLayer VuoLayer_makeFromString(const char *str);
char * VuoLayer_getString(const VuoLayer value);
void VuoLayer_retain(VuoLayer value);
void VuoLayer_release(VuoLayer value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
