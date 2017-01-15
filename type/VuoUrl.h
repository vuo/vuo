/**
 * @file
 * VuoUrl C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOURL_H
#define VUOURL_H

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
VuoUrl VuoUrl_normalize(const VuoText url, bool isSave);
VuoText VuoUrl_getPosixPath(const VuoUrl url);
VuoText VuoUrl_escapePosixPath(const VuoText posixPath);
bool VuoUrl_isBundle(const VuoUrl url);

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

#endif
