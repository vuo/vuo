/**
 * @file
 * VuoImageWrapMode C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoImageWrapMode_h
#define VuoImageWrapMode_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoImageWrapMode VuoImageWrapMode
 * An enum defining different types of image wrapping.
 *
 * @{
 */

/**
 * An enum defining different types of image wrapping.
 */
typedef enum {
	VuoImageWrapMode_None,			// pixels are rendered black
	VuoImageWrapMode_ClampEdge,		// the last pixel on the edge is repeated
	VuoImageWrapMode_Repeat,		// the image will tile
	VuoImageWrapMode_MirroredRepeat	// the image will repeat inverted
} VuoImageWrapMode;

#include "VuoList_VuoImageWrapMode.h"

VuoImageWrapMode VuoImageWrapMode_makeFromJson(struct json_object * js);
struct json_object * VuoImageWrapMode_getJson(const VuoImageWrapMode value);
VuoList_VuoImageWrapMode VuoImageWrapMode_getAllowedValues(void);
char * VuoImageWrapMode_getSummary(const VuoImageWrapMode value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoImageWrapMode_getString(const VuoImageWrapMode value);
void VuoImageWrapMode_retain(VuoImageWrapMode value);
void VuoImageWrapMode_release(VuoImageWrapMode value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif

#endif
