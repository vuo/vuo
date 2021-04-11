/**
 * @file
 * VuoRssItem implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include "VuoRssItem.h"
#include "VuoTime.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "RSS Feed Item",
					  "description" : "An item from an RSS feed.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoImage",
						  "VuoText",
						  "VuoList_VuoText",
						  "VuoTime",
						  "VuoUrl"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * An example from https://vuo.org/composition/feed :
 * @eg{
 *   {
 *     "title": "Parallax Scrolling",
 *     "author": "smokris",
 *     "description": "<p>A simple example of a few concepts:</p>…",  // UTF8 HTML text
 *     "url": "https://vuo.org/node/533",
 *     "image": {"glTextureName": 42, …}
 *   }
 * }
 */
VuoRssItem VuoRssItem_makeFromJson(json_object * js)
{
	return (VuoRssItem){
		VuoJson_getObjectValue(VuoText,         js, "title",       NULL),
		VuoJson_getObjectValue(VuoText,         js, "author",      NULL),
		VuoJson_getObjectValue(VuoText,         js, "description", NULL),
		VuoJson_getObjectValue(VuoText,         js, "url",         NULL),
		VuoJson_getObjectValue(VuoTime,         js, "dateTime",    NAN),
		VuoJson_getObjectValue(VuoText,         js, "imageUrl",    NULL),
		VuoJson_getObjectValue(VuoImage,        js, "image",       NULL),
		VuoJson_getObjectValue(VuoList_VuoText, js, "categories",  NULL)
	};
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoRssItem_getJson(const VuoRssItem value)
{
	json_object *js = json_object_new_object();

	json_object *titleObject = VuoText_getJson(value.title);
	json_object_object_add(js, "title", titleObject);

	json_object *authorObject = VuoText_getJson(value.author);
	json_object_object_add(js, "author", authorObject);

	json_object *descriptionObject = VuoText_getJson(value.description);
	json_object_object_add(js, "description", descriptionObject);

	json_object *urlObject = VuoUrl_getJson(value.url);
	json_object_object_add(js, "url", urlObject);

	json_object *dateTimeObject = VuoTime_getJson(value.dateTime);
	json_object_object_add(js, "dateTime", dateTimeObject);

	json_object *imageUrlObject = VuoUrl_getJson(value.imageUrl);
	json_object_object_add(js, "imageUrl", imageUrlObject);

	json_object *imageObject = VuoImage_getJson(value.image);
	json_object_object_add(js, "image", imageObject);

	json_object *categoriesObject = VuoList_VuoText_getJson(value.categories);
	json_object_object_add(js, "categories", categoriesObject);

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoRssItem_getSummary(const VuoRssItem value)
{
	return VuoText_getSummary(value.title);
}
