/**
 * @file
 * VuoMovieImageEncoding C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMOVIEIMAGEENCODING_H
#define VUOMOVIEIMAGEENCODING_H

/// @{
typedef const struct VuoList_VuoMovieImageEncoding_struct { void *l; } * VuoList_VuoMovieImageEncoding;
#define VuoList_VuoMovieImageEncoding_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoMovieImageEncoding VuoMovieImageEncoding
 * An enum defining different movie image encodings.
 *
 * @{
 */

/**
 * An enum defining different movie image encodings.
 */
typedef enum {
	VuoMovieImageEncoding_JPEG,
	VuoMovieImageEncoding_H264,
	VuoMovieImageEncoding_ProRes4444,
	VuoMovieImageEncoding_ProRes422
} VuoMovieImageEncoding;

VuoMovieImageEncoding VuoMovieImageEncoding_makeFromJson(struct json_object * js);
struct json_object * VuoMovieImageEncoding_getJson(const VuoMovieImageEncoding value);
VuoList_VuoMovieImageEncoding VuoMovieImageEncoding_getAllowedValues(void);
char * VuoMovieImageEncoding_getSummary(const VuoMovieImageEncoding value);

/// @{
/**
 * Automatically generated function.
 */
VuoMovieImageEncoding VuoMovieImageEncoding_makeFromString(const char *str);
char * VuoMovieImageEncoding_getString(const VuoMovieImageEncoding value);
void VuoMovieImageEncoding_retain(VuoMovieImageEncoding value);
void VuoMovieImageEncoding_release(VuoMovieImageEncoding value);
/// @}

/**
 * @}
*/

#endif
