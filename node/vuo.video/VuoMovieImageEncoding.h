/**
 * @file
 * VuoMovieImageEncoding C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoMovieImageEncoding VuoMovieImageEncoding
 * An enum defining different movie image encodings.
 *
 * @{
 */

/**
 * An enum defining different movie image encodings.
 * @version200Changed{Added `HEVC`, `HEVCAlpha`, `ProRes422HQ`, `ProRes422LT`, and `ProRes422Proxy`.}
 */
typedef enum {
	VuoMovieImageEncoding_JPEG,
	VuoMovieImageEncoding_H264,
	VuoMovieImageEncoding_ProRes4444,
	VuoMovieImageEncoding_ProRes422,
	VuoMovieImageEncoding_HEVC,
	VuoMovieImageEncoding_HEVCAlpha,
	VuoMovieImageEncoding_ProRes422HQ,
	VuoMovieImageEncoding_ProRes422LT,
	VuoMovieImageEncoding_ProRes422Proxy,
} VuoMovieImageEncoding;

#include "VuoList_VuoMovieImageEncoding.h"

VuoMovieImageEncoding VuoMovieImageEncoding_makeFromJson(struct json_object * js);
struct json_object * VuoMovieImageEncoding_getJson(const VuoMovieImageEncoding value);
VuoList_VuoMovieImageEncoding VuoMovieImageEncoding_getAllowedValues(void);
char * VuoMovieImageEncoding_getSummary(const VuoMovieImageEncoding value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoMovieImageEncoding_getString(const VuoMovieImageEncoding value);
void VuoMovieImageEncoding_retain(VuoMovieImageEncoding value);
void VuoMovieImageEncoding_release(VuoMovieImageEncoding value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif
