/**
 * @file
 * VuoAudioEncoding C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOAUDIOENCODING_H
#define VUOAUDIOENCODING_H

/// @{
typedef const struct VuoList_VuoAudioEncoding_struct { void *l; } * VuoList_VuoAudioEncoding;
#define VuoList_VuoAudioEncoding_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoAudioEncoding VuoAudioEncoding
 * An enum defining different audio encodings used when exporting movies.
 *
 * @{
 */

/**
 * An enum defining different audio encodings used when exporting movies.
 */
typedef enum {
	VuoAudioEncoding_LinearPCM,
	VuoAudioEncoding_AAC,
} VuoAudioEncoding;

VuoAudioEncoding VuoAudioEncoding_makeFromJson(struct json_object * js);
struct json_object * VuoAudioEncoding_getJson(const VuoAudioEncoding value);
VuoList_VuoAudioEncoding VuoAudioEncoding_getAllowedValues(void);
char * VuoAudioEncoding_getSummary(const VuoAudioEncoding value);

/// @{
/**
 * Automatically generated function.
 */
VuoAudioEncoding VuoAudioEncoding_makeFromString(const char *str);
char * VuoAudioEncoding_getString(const VuoAudioEncoding value);
void VuoAudioEncoding_retain(VuoAudioEncoding value);
void VuoAudioEncoding_release(VuoAudioEncoding value);
/// @}

/**
 * @}
*/

#endif
