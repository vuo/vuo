/**
 * @file
 * VuoHumanHand C type definition.
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
 * @defgroup VuoHumanHand VuoHumanHand
 * Coordinates of landmarks on a human hand.
 *
 * @{
 */

/**
 * Coordinates of landmarks on a human hand.
 */
typedef const struct { void *l; } * VuoHumanHand;

VuoHumanHand VuoHumanHand_makeFromJson(struct json_object *js);
struct json_object *VuoHumanHand_getJson(const VuoHumanHand value);
char *VuoHumanHand_getSummary(const VuoHumanHand value);

/// @{
/**
 * Automatically generated functions.
 */
char *VuoHumanHand_getString(const VuoHumanHand value);
void VuoHumanHand_retain(VuoHumanHand value);
void VuoHumanHand_release(VuoHumanHand value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
