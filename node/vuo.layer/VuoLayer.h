/**
 * @file
 * VuoLayer C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOLAYER_H
#define VUOLAYER_H

#include "VuoColor.h"
#include "VuoTransform2d.h"
#include "VuoSceneObject.h"

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
VuoLayer VuoLayer_make(VuoImage image, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal alpha);
VuoLayer VuoLayer_makeRealSize(VuoImage image, VuoPoint2d center, VuoReal alpha);
VuoLayer VuoLayer_makeColor(VuoColor color, VuoPoint2d center, VuoReal rotation, VuoReal width, VuoReal height);
VuoLayer VuoLayer_makeGroup(VuoList_VuoLayer childLayers, VuoTransform2d transform);

VuoLayer VuoLayer_valueFromJson(struct json_object * js);
struct json_object * VuoLayer_jsonFromValue(const VuoLayer value);
char * VuoLayer_summaryFromValue(const VuoLayer value);

///@{
/**
 * Automatically generated function.
 */
VuoLayer VuoLayer_valueFromString(const char *str);
char * VuoLayer_stringFromValue(const VuoLayer value);
void VuoLayer_retain(VuoLayer value);
void VuoLayer_release(VuoLayer value);
///@}

/**
 * @}
 */

#endif
