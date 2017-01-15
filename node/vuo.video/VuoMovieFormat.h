/**
 * @file
 * VuoMovieFormat C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMOVIEFORMAT_H
#define VUOMOVIEFORMAT_H

#include "VuoMovieImageEncoding.h"
#include "VuoAudioEncoding.h"
#include "VuoReal.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoMovieFormat VuoMovieFormat
 * Describes the attributes of a movie file.
 *
 * @{
 */

/**
 * Describes the attributes of a movie file.
 */
typedef struct
{
	VuoMovieImageEncoding imageEncoding;	///< Image encoding format.  JPEG, H.264, ProRes, etc.
	VuoReal imageQuality;					///< Image quality. 1 is best, 0 is worst.

	VuoAudioEncoding audioEncoding;			///< Audio encoding format.
	VuoReal audioQuality;					///< Audio quality.  1 is best, 0 is worst.
} VuoMovieFormat;

VuoMovieFormat VuoMovieFormat_makeFromJson(struct json_object * js);
struct json_object * VuoMovieFormat_getJson(const VuoMovieFormat value);
char * VuoMovieFormat_getSummary(const VuoMovieFormat value);
bool VuoMovieFormat_areEqual(VuoMovieFormat value1, VuoMovieFormat value2);

/**
 * Automatically generated function.
 */
///@{
VuoMovieFormat VuoMovieFormat_makeFromString(const char *str);
char * VuoMovieFormat_getString(const VuoMovieFormat value);
void VuoMovieFormat_retain(VuoMovieFormat value);
void VuoMovieFormat_release(VuoMovieFormat value);
///@}

/**
 * Returns a new VuoMovieFormat with specified attributes.
 */
static inline VuoMovieFormat VuoMovieFormat_make(	VuoMovieImageEncoding imgEncoding,
													VuoReal imgQuality,
													VuoAudioEncoding audEncoding,
													VuoReal audQuality) __attribute__((const));
static inline VuoMovieFormat VuoMovieFormat_make(	VuoMovieImageEncoding imgEncoding,
													VuoReal imgQuality,
													VuoAudioEncoding audEncoding,
													VuoReal audQuality)
{
	VuoMovieFormat fmt = { imgEncoding, imgQuality, audEncoding, audQuality };
	return fmt;
};

/**
 * @}
 */

#endif // VuoMovieFormat_H

