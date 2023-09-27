/**
 * @file
 * VuoRssItem C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoRssItem VuoRssItem
 * An item from an RSS feed.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoText.h"
#include "VuoImage.h"
#include "VuoUrl.h"
#include "VuoList_VuoText.h"

/**
 * An item from an RSS feed.
 */
typedef struct
{
	VuoText title;
	VuoText author;
	VuoText description;
	VuoUrl url;
	double /*VuoTime*/ dateTime;
	VuoUrl imageUrl;
	VuoImage image;
	VuoList_VuoText categories;
} VuoRssItem;

VuoRssItem VuoRssItem_makeFromJson(struct json_object * js);
struct json_object * VuoRssItem_getJson(const VuoRssItem value);
char * VuoRssItem_getSummary(const VuoRssItem value);

/**
 * Automatically generated function.
 */
///@{
char * VuoRssItem_getString(const VuoRssItem value);
void VuoRssItem_retain(VuoRssItem value);
void VuoRssItem_release(VuoRssItem value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
