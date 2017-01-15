/**
 * @file
 * VuoRssItem C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORSSITEM_H
#define VUORSSITEM_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoRssItem VuoRssItem
 * An item from an RSS feed.
 *
 * @{
 */

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
VuoRssItem VuoRssItem_makeFromString(const char *str);
char * VuoRssItem_getString(const VuoRssItem value);
void VuoRssItem_retain(VuoRssItem value);
void VuoRssItem_release(VuoRssItem value);
///@}

/**
 * @}
 */

#endif // VUORSSITEM_H

