/**
 * @file
 * VuoLayer C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOLAYER_H
#define VUOLAYER_H

#include "VuoColor.h"
#include "VuoPoint2d.h"
#include "VuoSceneObject.h"
#include "VuoTransform2d.h"
#include "VuoWindowReference.h"
#include "VuoList_VuoColor.h"

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
 * @todo Ideally this should just be `typedef VuoSceneObject VuoLayer`, but https://b33p.net/kosada/node/6824
 */
typedef struct VuoLayer
{
	VuoSceneObject sceneObject;	///< A VuoLayer is secretly just a VuoSceneObject.
} VuoLayer;

#include "VuoList_VuoLayer.h"

VuoLayer VuoLayer_makeEmpty(void);
VuoLayer VuoLayer_makeGroup(VuoList_VuoLayer childLayers, VuoTransform2d transform);

VuoLayer VuoLayer_make(VuoText name, VuoImage image, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal alpha);
VuoLayer VuoLayer_makeWithShadow(VuoText name, VuoImage image, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal alpha, VuoColor shadowColor, VuoReal shadowBlur, VuoReal shadowAngle, VuoReal shadowDistance);
VuoLayer VuoLayer_makeRealSize(VuoText name, VuoImage image, VuoPoint2d center, VuoReal alpha);
VuoLayer VuoLayer_makeRealSizeWithShadow(VuoText name, VuoImage image, VuoPoint2d center, VuoReal alpha, VuoColor shadowColor, VuoReal shadowBlur, VuoReal shadowAngle, VuoReal shadowDistance);

VuoLayer VuoLayer_makeColor(VuoText name, VuoColor color, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height);
VuoLayer VuoLayer_makeOval(VuoText name, VuoColor color, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height, VuoReal sharpness);
VuoLayer VuoLayer_makeRoundedRectangle(VuoText name, VuoColor color, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height, VuoReal sharpness, VuoReal roundness);

VuoLayer VuoLayer_makeLinearGradient(VuoText name, VuoList_VuoColor colors, VuoPoint2d start, VuoPoint2d end, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height, VuoReal noiseAmount);
VuoLayer VuoLayer_makeRadialGradient(VuoText name, VuoList_VuoColor colors, VuoPoint2d gradientCenter, VuoReal radius, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height, VuoReal noiseAmount);

VuoRectangle VuoLayer_getBoundingRectangle(VuoLayer layer, VuoInteger viewportWidth, VuoInteger viewportHeight, float backingScaleFactor);

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

#endif
