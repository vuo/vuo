/**
 * @file
 * VuoWindowReference interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoWindowReference_h
#define VuoWindowReference_h

/**
 * @ingroup VuoTypes
 * @defgroup VuoWindowReference VuoWindowReference
 * A unique ID for a window being displayed by the composition.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoInteger.h"
#include "VuoReal.h"
#include "VuoPoint2d.h"
#include "VuoScreen.h"

/**
 * A unique ID for a window being displayed by the composition.
 */
typedef const struct VuoWindowReference_struct { void *l; } *VuoWindowReference;

#define VuoWindowReference_OVERRIDES_INTERPROCESS_SERIALIZATION  ///< This type implements `_getInterprocessJson()`.

VuoWindowReference VuoWindowReference_make(void *window);

VuoWindowReference VuoWindowReference_makeFromJson(struct json_object * js);
struct json_object * VuoWindowReference_getJson(const VuoWindowReference value);
struct json_object * VuoWindowReference_getInterprocessJson(const VuoWindowReference value);
char * VuoWindowReference_getSummary(const VuoWindowReference value);

VuoReal VuoWindowReference_getAspectRatio(const VuoWindowReference value);
VuoPoint2d VuoWindowReference_getPosition(const VuoWindowReference value);
void VuoWindowReference_getContentSize(const VuoWindowReference value, VuoInteger *width, VuoInteger *height, float *backingScaleFactor);
bool VuoWindowReference_isFocused(const VuoWindowReference value);
bool VuoWindowReference_isFullscreen(const VuoWindowReference value);
VuoScreen VuoWindowReference_getScreen(const VuoWindowReference value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoWindowReference_getString(const VuoWindowReference value);
void VuoWindowReference_retain(VuoWindowReference value);
void VuoWindowReference_release(VuoWindowReference value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
