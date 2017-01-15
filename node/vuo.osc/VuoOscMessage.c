/**
 * @file
 * VuoOscMessage implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoOscMessage.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "OSC Message",
					 "description" : "An OSC message.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoText"
					 ]
				 });
#endif
/// @}


/**
 * @ingroup VuoOscMessage
 * Releases the @c message's values and frees the @c message object.
 *
 * @threadAny
 */
void VuoOscMessage_free(void *message)
{
	VuoOscMessage o = (VuoOscMessage)message;
	VuoRelease(o->address);
	json_object_put(o->data);
	free(o);
}

/**
 * @ingroup VuoOscMessage
 * Creates an OSC message having the specified @c address and carrying the specified @c data.
 * @c data should be a JSON array, and can contain zero or more values of any type.
 *
 * The VuoOscMessage takes ownership of @c data (expected to have json_object retain count 1);
 * the caller should not @c json_object_put it, or modify it after calling this function.
 */
VuoOscMessage VuoOscMessage_make(VuoText address, json_object *data)
{
	VuoOscMessage o = (VuoOscMessage)malloc(sizeof(struct _VuoOscMessage));
	VuoRegister(o, VuoOscMessage_free);

	o->address = address;
	VuoRetain(o->address);

	o->data = data;

	return o;
}

/**
 * @ingroup VuoOscMessage
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "address" : "/foo",
 *     "data" : [ ... ]
 *   }
 * }
 */
VuoOscMessage VuoOscMessage_makeFromJson(json_object * js)
{
	if (!js)
		return NULL;

	json_object *o = NULL;

	VuoText address = NULL;
	if (json_object_object_get_ex(js, "address", &o))
		address = VuoText_makeFromJson(o);

	json_object *data = NULL;
	if (json_object_object_get_ex(js, "data", &o))
		data = json_object_get(o);

	return VuoOscMessage_make(address, data);
}

/**
 * @ingroup VuoOscMessage
 * Encodes @c value as a JSON object.
 */
json_object * VuoOscMessage_getJson(const VuoOscMessage value)
{
	json_object *js = json_object_new_object();

	if (!value)
		return NULL;

	if (value->address)
		json_object_object_add(js, "address", json_object_new_string(value->address));

	if (value->data)
		json_object_object_add(js, "data", json_object_get(value->data));

	return js;
}

/// How many OSC values should the summary include?
#define SUMMARY_ITEMS_TO_SHOW 4

/**
 * @ingroup VuoOscMessage
 * Produces a brief human-readable summary of @c value.
 */
char * VuoOscMessage_getSummary(const VuoOscMessage value)
{
	if (!value)
		return strdup("(no message)");

	char *data[SUMMARY_ITEMS_TO_SHOW];
	int dataCount = 0;
	if (value->data)
	{
		dataCount = json_object_array_length(value->data);
		for (int i=0; i<MIN(SUMMARY_ITEMS_TO_SHOW,dataCount); ++i)
		{
			json_object *o = json_object_array_get_idx(value->data, i);
			switch (json_object_get_type(o))
			{
				case json_type_null:
					data[i] = strdup("null");
					break;
				case json_type_boolean:
					data[i] = strdup(json_object_get_boolean(o) ? "true" : "false");
					break;
				case json_type_double:
				{
					double v = json_object_get_double(o);
					data[i] = VuoText_format("%g", v);
					break;
				}
				case json_type_int:
				{
					long long v = json_object_get_int64(o);
					data[i] = VuoText_format("%lld", v);
					break;
				}
				case json_type_string:
					data[i] = strdup(json_object_get_string(o));
					break;
				default:
					data[i] = strdup("?");
			}
		}
	}

	int dataSize = 0;
	for (int i=0; i<MIN(SUMMARY_ITEMS_TO_SHOW,dataCount); ++i)
		dataSize += strlen(data[i]);
	if (dataCount > 1)
		dataSize += (MIN(SUMMARY_ITEMS_TO_SHOW,dataCount) - 1) * strlen(", ");
	if (dataCount > SUMMARY_ITEMS_TO_SHOW)
		dataSize += strlen(", ...");

	char *compositeData = (char *)malloc(dataSize+1);
	compositeData[0] = 0;
	for (int i=0; i<MIN(SUMMARY_ITEMS_TO_SHOW,dataCount); ++i)
	{
		if (i>0)
			strcat(compositeData, ", ");
		strcat(compositeData, data[i]);
	}
	if (dataCount > SUMMARY_ITEMS_TO_SHOW)
		strcat(compositeData, ", ...");

	char *valueAsString = VuoText_format("%s<br>[ %s ]", value->address, compositeData);

	free(compositeData);
	for (int i=0; i<MIN(SUMMARY_ITEMS_TO_SHOW,dataCount); ++i)
		free(data[i]);

	return valueAsString;
}

/**
 * Returns the number of data values in the OSC message.
 */
int VuoOscMessage_getDataCount(const VuoOscMessage value)
{
	return json_object_array_length(value->data);
}

/**
 * Returns a JSON object representing the OSC message's data value at @c index.
 * The @c index values start at 1.
 */
struct json_object * VuoOscMessage_getDataJson(const VuoOscMessage value, int index)
{
	return json_object_array_get_idx(value->data, index-1);
}
