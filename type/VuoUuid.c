/**
 * @file
 * VuoUuid implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoUuid.h"
#include "VuoText.h"
#include <uuid/uuid.h>

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "UUID",
					 "description" : "Universally Unique Identifier.",
					 "keywords" : [ "guid", "unique", "tag" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoText",
						"VuoList_VuoUuid"
					 ]
				 });
#endif
/// @}

/**
 * Returns a new UUID.
 *
 * @version200New
 */
VuoUuid VuoUuid_make(void)
{
	VuoUuid id;
	uuid_generate(id.bytes);
	return id;
}

/**
 * @ingroup VuoUuid
 * Decodes the JSON object @c js.
 *
 * @version200New
 */
VuoUuid VuoUuid_makeFromJson(json_object *js)
{
	VuoUuid id;
	bzero(&id, sizeof(VuoUuid));

	if (json_object_get_type(js) == json_type_array)
	{
		for (int i = 0; i < 16; ++i)
		{
			json_object* index = json_object_array_get_idx(js, i);
			int64_t value = json_object_get_int64(index);
			id.bytes[i] = value;
		}
	}

	return id;
}

/**
 * @ingroup VuoUuid
 * Encodes @c value as a JSON object.
 *
 * @version200New
 */
json_object * VuoUuid_getJson(const VuoUuid value)
{
	json_object *bytes = json_object_new_array();

	for (unsigned int i = 0; i < 16; i++)
	{
		json_object* o = json_object_new_int64(value.bytes[i]);
		json_object_array_add(bytes, o);
	}

	return bytes;
}

/**
 * @ingroup VuoUuid
 * Show the hex values for each byte.
 *
 * @version200New
 */
char * VuoUuid_getSummary(const VuoUuid value)
{
	return VuoText_format("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
							value.bytes[ 0],
							value.bytes[ 1],
							value.bytes[ 2],
							value.bytes[ 3],
							value.bytes[ 4],
							value.bytes[ 5],
							value.bytes[ 6],
							value.bytes[ 7],
							value.bytes[ 8],
							value.bytes[ 9],
							value.bytes[10],
							value.bytes[11],
							value.bytes[12],
							value.bytes[13],
							value.bytes[14],
							value.bytes[15] );
}
