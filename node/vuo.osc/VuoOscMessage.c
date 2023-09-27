/**
 * @file
 * VuoOscMessage implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoOscMessage.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "OSC Message",
					 "description" : "An OSC message.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoOscType",
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

	for (unsigned int i = 0; i < o->dataCount; ++i)
		json_object_put(o->data[i]);

	free(o);
}

/**
 * @ingroup VuoOscMessage
 * Creates an OSC message having the specified @c address and carrying the specified @c data.
 * @c data should be a JSON array, and can contain zero or more values of any type.
 *
 * The VuoOscMessage takes ownership of the `json_object`s in `data` (expected to have json_object retain count 1);
 * the caller should not `json_object_put` the objects, or modify them after calling this function.
 */
VuoOscMessage VuoOscMessage_make(VuoText address, unsigned int dataCount, struct json_object **data, VuoOscType *dataTypes)
{
	VuoOscMessage o = (VuoOscMessage)malloc(sizeof(struct _VuoOscMessage));
	VuoRegister(o, VuoOscMessage_free);

	o->address = address;
	VuoRetain(o->address);

	if (dataCount > VUOOSC_MAX_MESSAGE_ARGUMENTS)
		VUserLog("Warning: OSC message has more than %d arguments; ignoring extras.", VUOOSC_MAX_MESSAGE_ARGUMENTS);

	o->dataCount = MIN(dataCount, VUOOSC_MAX_MESSAGE_ARGUMENTS);
	if (dataCount > 0)
	{
		memcpy(o->data,      data,      sizeof(struct json_object *) * o->dataCount);
		memcpy(o->dataTypes, dataTypes, sizeof(VuoOscType)           * o->dataCount);
	}

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

	unsigned int dataCount = 0;
	json_object *data[VUOOSC_MAX_MESSAGE_ARGUMENTS];
	VuoOscType dataTypes[VUOOSC_MAX_MESSAGE_ARGUMENTS];
	if (json_object_object_get_ex(js, "data", &o))
	{
		dataCount = json_object_array_length(o);
		for (unsigned int i = 0; i < dataCount; ++i)
		{
			json_object *di = json_object_array_get_idx(o, i);
			json_object *v;
			if (json_object_object_get_ex(di, "type", &v))
				dataTypes[i] = VuoOscType_makeFromJson(v);
			if (json_object_object_get_ex(di, "data", &v))
				data[i] = json_object_get(v);
		}
	}

	return VuoOscMessage_make(address, dataCount, data, dataTypes);
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

	if (value->dataCount)
	{
		struct json_object *data = json_object_new_array();
		for (unsigned int i = 0; i < value->dataCount; ++i)
		{
			struct json_object *v = json_object_new_object();
			json_object_object_add(v, "type", VuoOscType_getJson(value->dataTypes[i]));
			json_object_object_add(v, "data", json_object_get(value->data[i]));
			json_object_array_add(data, v);
		}
		json_object_object_add(js, "data", data);
	}

	return js;
}

/**
 * @ingroup VuoOscMessage
 * Produces a brief human-readable summary of @c value.
 */
char * VuoOscMessage_getSummary(const VuoOscMessage value)
{
	if (!value)
		return strdup("No message");

	int dataCount = value->dataCount;
	char *data[dataCount];
	if (dataCount)
	{
		for (int i = 0; i < dataCount; ++i)
		{
			json_object *o = value->data[i];
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
	for (int i = 0; i < dataCount; ++i)
		dataSize += strlen(data[i]);
	if (dataCount > 1)
		dataSize += (dataCount - 1) * strlen(", ");
	dataSize += 1;  // Null terminator

	char *compositeData = (char *)malloc(dataSize);
	compositeData[0] = 0;
	for (int i = 0; i < dataCount; ++i)
	{
		if (i>0)
			strlcat(compositeData, ", ", dataSize);
		strlcat(compositeData, data[i], dataSize);
	}

	char *valueAsString = VuoText_format("%s<br>[ %s ]", value->address, compositeData);

	free(compositeData);
	for (int i = 0; i < dataCount; ++i)
		free(data[i]);

	return valueAsString;
}
