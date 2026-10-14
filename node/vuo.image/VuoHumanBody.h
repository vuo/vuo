/**
 * @file
 * VuoHumanBody C type definition.
 *
 * @copyright Copyright © 2012–2026 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoHumanBody VuoHumanBody
 * Coordinates of landmarks on a human body.
 *
 * @{
 */

/**
 * Coordinates of landmarks on a human body.
 */
typedef const struct { void *l; } * VuoHumanBody;

VuoHumanBody VuoHumanBody_makeFromJson(struct json_object *js);
struct json_object *VuoHumanBody_getJson(const VuoHumanBody value);
char *VuoHumanBody_getSummary(const VuoHumanBody value);

/// @{
/**
 * Automatically generated functions.
 */
char *VuoHumanBody_getString(const VuoHumanBody value);
void VuoHumanBody_retain(VuoHumanBody value);
void VuoHumanBody_release(VuoHumanBody value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
