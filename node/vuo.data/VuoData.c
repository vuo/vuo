/**
 * @file
 * VuoData implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include "VuoData.h"
#include "VuoBase64.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Binary Data",
					  "description" : "A blob of 8-bit binary data.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoBase64",
						  "VuoText"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object `js` to create a new value.
 *
 * `js` is expected to be a JSON string object containing base64-encoded data.
 */
VuoData VuoData_makeFromJson(json_object *js)
{
	VuoData value = {0, NULL};

	if (json_object_get_type(js) != json_type_string)
		return value;

	value.data = VuoBase64_decode(json_object_get_string(js), &value.size);
	VuoRegister(value.data, free);
	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoData_getJson(const VuoData value)
{
	if (!value.data)
		return NULL;

	char *encoded = VuoBase64_encode(value.size, value.data);
	json_object *js = json_object_new_string(encoded);
	free(encoded);
	return js;
}

/**
 * Returns true if the two values are equal.
 */
bool VuoData_areEqual(const VuoData valueA, const VuoData valueB)
{
	if (valueA.size != valueB.size)
		return false;

	if (!valueA.data || !valueB.data)
		return (!valueA.data && !valueB.data);

	return memcmp(valueA.data, valueB.data, valueA.size) == 0;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoData_isLessThan(const VuoData valueA, const VuoData valueB)
{
	// Treat null data as greater than non-null data,
	// so the more useful non-null data sorts to the beginning of the list.
	if (!valueA.data || !valueB.data)
		return valueA.data && !valueB.data;

	int prefixCmp = memcmp(valueA.data, valueB.data, MIN(valueA.size, valueB.size));
	if (!prefixCmp)
		return (valueA.size < valueB.size);
	else
		return (prefixCmp < 0);
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoData_getSummary(const VuoData value)
{
	if (value.data)
		return VuoText_format("%lld byte%s of data", value.size, value.size == 1 ? "" : "s");
	else
		return strdup("no data");
}

/**
 * Returns a Binary Data structure with the specified values.
 *
 * `data` becomes owned by the returned VuoData;
 * it will be `free`d after the final @ref VuoRelease is called;
 * it should not be modified after calling this function.
 */
VuoData VuoData_make(VuoInteger size, unsigned char *data)
{
	VuoData value;
	value.size = size;
	value.data = (char *)data;

	if (data)
		VuoRegister(value.data, free);

	return value;
}

/**
 * Returns a Binary Data structure with the specified text data (excluding the text's trailing NULL).
 *
 * `text` is copied.
 */
VuoData VuoData_makeFromText(const VuoText text)
{
	VuoData value;
	value.size = strlen(text);
	value.data = (char *)malloc(value.size);
	memcpy(value.data, text, value.size);
	VuoRegister(value.data, free);
	return value;
}

/**
 * Returns the string of characters stored in @a data, plus a trailing NULL.
 *
 * The string of characters is copied.
 */
char * VuoData_copyToString(const VuoData data)
{
	char *dataAsString = (char *)malloc(data.size + 1);
	dataAsString[data.size] = 0;
	memcpy(dataAsString, data.data, data.size);
	return dataAsString;
}

/**
 * `VuoCompilerType::parseOrGenerateRetainOrReleaseFunction` can't currently generate this on arm64.
 * https://b33p.net/kosada/vuo/vuo/-/issues/19142#note_2158967
 */
void VuoData_retain(VuoData value)
{
	VuoRetain(value.data);
}

/**
 * `VuoCompilerType::parseOrGenerateRetainOrReleaseFunction` can't currently generate this on arm64.
 * https://b33p.net/kosada/vuo/vuo/-/issues/19142#note_2158967
 */
void VuoData_release(VuoData value)
{
	VuoRelease(value.data);
}
