/**
 * @file
 * VuoImageColorDepth C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoImageColorDepth_h
#define VuoImageColorDepth_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoImageColorDepth VuoImageColorDepth
 * An image's color bit-depth
 *
 * @{
 */

/**
 * An image's color bit-depth
 */
typedef enum {
	VuoImageColorDepth_8,			//  8 bits per channel (32 bits per RGBA pixel)
	VuoImageColorDepth_16,			// 16 bits per channel (64 bits per RGBA pixel)
	VuoImageColorDepth_32,			// 32 bits per channel (128 bits per RGBA pixel)
} VuoImageColorDepth;

#include "VuoList_VuoImageColorDepth.h"

unsigned int VuoImageColorDepth_getGlInternalFormat(unsigned int baseFormat, VuoImageColorDepth imageColorDepth);

VuoImageColorDepth VuoImageColorDepth_makeFromJson(struct json_object * js);
struct json_object * VuoImageColorDepth_getJson(const VuoImageColorDepth value);
VuoList_VuoImageColorDepth VuoImageColorDepth_getAllowedValues(void);
char * VuoImageColorDepth_getSummary(const VuoImageColorDepth value);

/**
 * Automatically generated function.
 */
///@{
char * VuoImageColorDepth_getString(const VuoImageColorDepth value);
void VuoImageColorDepth_retain(VuoImageColorDepth value);
void VuoImageColorDepth_release(VuoImageColorDepth value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
