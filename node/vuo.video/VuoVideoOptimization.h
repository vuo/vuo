/**
 * @file
 * VuoVideoOptimization C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOVIDEOOPTIMIZATION_H
#define VUOVIDEOOPTIMIZATION_H

/// @{
typedef const struct VuoList_VuoVideoOptimization_struct { void *l; } * VuoList_VuoVideoOptimization;
#define VuoList_VuoVideoOptimization_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoVideoOptimization VuoVideoOptimization
 * An enum defining different video decoder quality settings (prefer forward playback or random decode performance).
 *
 * @{
 */

/**
 * An enum defining different video decoder quality settings (prefer forward playback or random decode performance).
 * When set to Auto, VuoVideoPlayer will automatically choose the decoder to use based on the current video player
 * settings.  When set to Forward AvFoundation is preferred over Ffmpeg, and vice-versa for Random.
 */
typedef enum {
	VuoVideoOptimization_Auto,
	VuoVideoOptimization_Forward,
	VuoVideoOptimization_Random
} VuoVideoOptimization;

VuoVideoOptimization VuoVideoOptimization_makeFromJson(struct json_object * js);
struct json_object * VuoVideoOptimization_getJson(const VuoVideoOptimization value);
VuoList_VuoVideoOptimization VuoVideoOptimization_getAllowedValues(void);
char * VuoVideoOptimization_getSummary(const VuoVideoOptimization value);

/// @{
/**
 * Automatically generated function.
 */
VuoVideoOptimization VuoVideoOptimization_makeFromString(const char *str);
char * VuoVideoOptimization_getString(const VuoVideoOptimization value);
void VuoVideoOptimization_retain(VuoVideoOptimization value);
void VuoVideoOptimization_release(VuoVideoOptimization value);
/// @}

/**
 * @}
*/

#endif
