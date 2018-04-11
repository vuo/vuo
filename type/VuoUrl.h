/**
 * @file
 * VuoUrl C type definition.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoUrl VuoUrl
 * Uniform Resource Locator.
 *
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoInteger.h"
#include "VuoText.h"

struct json_object;

/**
 * Uniform Resource Locator.
 */
typedef VuoText VuoUrl;

VuoUrl VuoUrl_makeFromJson(struct json_object *js);
struct json_object *VuoUrl_getJson(const VuoUrl value);
char * VuoUrl_getSummary(const VuoUrl value);
bool VuoUrl_getParts(const VuoUrl url, VuoText *scheme, VuoText *user, VuoText *host, VuoInteger *port, VuoText *path, VuoText *query, VuoText *fragment);
bool VuoUrl_getFileParts(const VuoUrl url, VuoText *path, VuoText *folder, VuoText *filename, VuoText *extension);

#define VuoUrl_SUPPORTS_COMPARISON
bool VuoUrl_areEqual(const VuoText a, const VuoText b);
bool VuoUrl_isLessThan(const VuoText a, const VuoText b);

bool VuoUrl_isRelativePath(const VuoUrl url);

/**
 * Bitwise flags modifying URL normalization behavior.
 */
enum VuoUrlNormalizeFlags {
	VuoUrlNormalize_default           = 0x0, ///< Standard normalization (none of the below flags set).
	VuoUrlNormalize_forSaving         = 0x1, ///< For exported apps: if set, relative file paths will be resolved to Desktop instead of the app resources folder.
	VuoUrlNormalize_assumeHttp        = 0x2, ///< If the URL doesn't have an explicit scheme: if unset, the `file` scheme is added.  If set, the `http` scheme is added.
};
VuoUrl VuoUrl_normalize(const VuoText url, enum VuoUrlNormalizeFlags flags);

VuoText VuoUrl_getPosixPath(const VuoUrl url);
VuoText VuoUrl_escapePosixPath(const VuoText posixPath);
bool VuoUrl_isBundle(const VuoUrl url);
VuoUrl VuoUrl_appendFileExtension(const char *filename, const char** validExtensions, const unsigned int extensionsLength);
VuoText VuoUrl_decodeRFC3986(const VuoUrl url);

/// @{
/**
 * Automatically generated function.
 */
VuoUrl VuoUrl_makeFromString(const char *str);
char *VuoUrl_getString(const VuoUrl value);
void VuoUrl_retain(VuoUrl value);
void VuoUrl_release(VuoUrl value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
