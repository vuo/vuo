/**
 * @file
 * VuoImageFormat C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOIMAGEFORMAT_H
#define VUOIMAGEFORMAT_H

/// @{
typedef const struct VuoList_VuoImageFormat_struct { void *l; } * VuoList_VuoImageFormat;
#define VuoList_VuoImageFormat_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoImageFormat VuoImageFormat
 * An enum defining different image formats that Vuo is capable of exporting.
 *
 * @{
 */

/**
 * An enum defining different image formats that Vuo is capable of exporting.
 */
typedef enum {
	VuoImageFormat_PNG,
	VuoImageFormat_JPEG,
	VuoImageFormat_TIFF,
	VuoImageFormat_BMP,
	VuoImageFormat_HDR,
	VuoImageFormat_EXR,
	VuoImageFormat_GIF,
	VuoImageFormat_TARGA
	// VuoImageFormat_WEBP // @todo https://b33p.net/kosada/node/10022
} VuoImageFormat;

VuoImageFormat VuoImageFormat_makeFromJson(struct json_object * js);
struct json_object * VuoImageFormat_getJson(const VuoImageFormat value);
VuoList_VuoImageFormat VuoImageFormat_getAllowedValues(void);
char * VuoImageFormat_getSummary(const VuoImageFormat value);
char** VuoImageFormat_getValidFileExtensions(const VuoImageFormat value, int* length);
char * VuoImageFormat_getExtension(const VuoImageFormat value);

/// @{
/**
 * Automatically generated function.
 */
VuoImageFormat VuoImageFormat_makeFromString(const char *str);
char * VuoImageFormat_getString(const VuoImageFormat value);
void VuoImageFormat_retain(VuoImageFormat value);
void VuoImageFormat_release(VuoImageFormat value);
/// @}

/**
 * @}
*/

#endif
