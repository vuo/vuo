/**
 * @file
 * VuoSceneText interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoSceneObject.h"
#include "VuoText.h"
#include "VuoFont.h"
#include "VuoAnchor.h"

/**
 * Create a new VuoSceneObject set up with the correct font, shader, and mesh anchoring for text.
 */
VuoSceneObject VuoSceneText_make(const VuoText text, const VuoFont font, const VuoAnchor anchor);

#ifdef __cplusplus
}
#endif
