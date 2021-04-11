/**
 * @file
 * VuoSceneText interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoSceneObject.h"
#include "VuoText.h"
#include "VuoFont.h"
#include "VuoAnchor.h"

VuoSceneObject VuoSceneText_make(const VuoText text, const VuoFont font, const VuoBoolean scaleWithScene, const VuoReal wrapWidth, const VuoAnchor anchor);
VuoAnchor VuoSceneText_getAnchor(VuoSceneObject so);
VuoPoint2d VuoSceneText_getAnchorOffset(VuoSceneObject so, float verticalScale, float rotationZ, float wrapWidth, int viewportWidth, int backingScaleFactor);

#ifdef __cplusplus
}
#endif
