/**
 * @file
 * VuoEdgeBlend C type definition.
 *
 * @copyright Copyright © 2012–2024 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoEdgeBlend VuoEdgeBlend
 * Apply a crop, fade, and gamma curve to an edge.
 *
 * @{
 */

/**
 * Apply a crop, fade, and gamma curve to an edge.
 */
typedef struct
{
	float cutoff;  ///< The proportion of the image (typically 0 to 0.5) that should be blended from transparent to the image's full opacity.
	float gamma;   ///< Controls the gradient of the blending. As gamma increases, the change from transparent to full opacity becomes more gradual.
	float crop;    ///< The proportion of the image (typically 0 to 0.5) that should be cropped out. When cropped, the image is stretched so that it still reaches the corner points.
} VuoEdgeBlend;

/**
 * Creates a VuoEdgeBlend structure.
 */
static inline VuoEdgeBlend VuoEdgeBlend_make(float _cutoff, float _gamma, float _crop) __attribute__((const));
static inline VuoEdgeBlend VuoEdgeBlend_make(float _cutoff, float _gamma, float _crop)
{
	VuoEdgeBlend blend = { _cutoff, _gamma, _crop};
	return blend;
}

VuoEdgeBlend VuoEdgeBlend_makeFromJson(struct json_object * js);
struct json_object * VuoEdgeBlend_getJson(const VuoEdgeBlend value);
char * VuoEdgeBlend_getSummary(const VuoEdgeBlend value);

///@{
/**
 * Automatically generated function.
 */
char * VuoEdgeBlend_getString(const VuoEdgeBlend value);
void VuoEdgeBlend_retain(VuoEdgeBlend value);
void VuoEdgeBlend_release(VuoEdgeBlend value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
