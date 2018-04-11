/**
 * @file
 * VuoDictionary_VuoText_VuoText implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "type.h"
#include "VuoDictionary_VuoText_VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Dictionary of Text keys and Text values",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoText",
						"VuoText",
						"VuoList_VuoText",
						"VuoList_VuoText"
					  ]
				  });
#endif
/// @}

/**
 * @ingroup VuoDictionary_VuoText_VuoText
 * Decodes the JSON object to create a new value.
 *
 * @eg{
 *   {
 *     "keys" : ["a","b","c"],
 *     "values" : [1.2, 3.4, 5.6]
 *   }
 * }
 */
VuoDictionary_VuoText_VuoText VuoDictionary_VuoText_VuoText_makeFromJson(json_object *js)
{
	VuoDictionary_VuoText_VuoText d;
	json_object *o = NULL;

	bool hasKeys = json_object_object_get_ex(js, "keys", &o);
	d.keys = VuoList_VuoText_makeFromJson(hasKeys ? o : NULL);

	bool hasValues = json_object_object_get_ex(js, "values", &o);
	d.values = VuoList_VuoText_makeFromJson(hasValues ? o : NULL);

	return d;
}

/**
 * @ingroup VuoDictionary_VuoText_VuoText
 * Encodes the value as a JSON object.
 */
json_object * VuoDictionary_VuoText_VuoText_getJson(const VuoDictionary_VuoText_VuoText d)
{
	json_object *js = json_object_new_object();

	json_object_object_add(js, "keys", VuoList_VuoText_getJson(d.keys));
	json_object_object_add(js, "values", VuoList_VuoText_getJson(d.values));

	return js;
}

/**
 * @ingroup VuoDictionary_VuoText_VuoText
 * Returns a brief description of the value.
 *
 * (When this type is replaced with a generic VuoDictionary type, this function will be replaced with
 * a nicer implementation, more like VuoList_getSummary().)
 */
char * VuoDictionary_VuoText_VuoText_getSummary(const VuoDictionary_VuoText_VuoText d)
{
	char *keysSummary = VuoList_VuoText_getSummary(d.keys);
	char *valuesSummary = VuoList_VuoText_getSummary(d.values);

	char *summary = VuoText_format("<ul><li>Keys: %s</li><li>Values: %s</li></ul>", keysSummary, valuesSummary);

	free(keysSummary);
	free(valuesSummary);

	return summary;
}

/**
 * @ingroup VuoDictionary_VuoText_VuoText
 * Creates an empty dictionary.
 */
VuoDictionary_VuoText_VuoText VuoDictionaryCreate_VuoText_VuoText(void)
{
	VuoDictionary_VuoText_VuoText d = { VuoListCreate_VuoText(), VuoListCreate_VuoText() };
	return d;
}

/**
 * Creates a dictionary consisting of the specified keys and values.
 */
VuoDictionary_VuoText_VuoText VuoDictionaryCreateWithLists_VuoText_VuoText(const VuoList_VuoText keys, const VuoList_VuoText values)
{
	return (VuoDictionary_VuoText_VuoText){keys, values};
}

/**
 * @ingroup VuoDictionary_VuoText_VuoText
 * Returns the value mapped from @a key in the dictionary.
 *
 * In order to match, `key` must use the same Unicode character compositions as the dictionary's key.
 */
VuoText VuoDictionaryGetValueForKey_VuoText_VuoText(VuoDictionary_VuoText_VuoText d, VuoText key)
{
	unsigned long count = VuoListGetCount_VuoText(d.keys);
	for (unsigned long i = 1; i <= count; ++i)
		if (strcmp(key, VuoListGetValue_VuoText(d.keys, i)) == 0)
			return VuoListGetValue_VuoText(d.values, i);

	return VuoText_make("");
}

/**
 * @ingroup VuoDictionary_VuoText_VuoText
 * Sets the value mapped from @a key in the dictionary to @a value.
 *
 * (Currently, this only allows adding a key-value mapping, not changing an existing one. When this type
 * is replaced with a generic VuoDictionary type, this function will allow the latter.)
 */
void VuoDictionarySetKeyValue_VuoText_VuoText(VuoDictionary_VuoText_VuoText d, VuoText key, VuoText value)
{
	VuoListAppendValue_VuoText(d.keys, key);
	VuoListAppendValue_VuoText(d.values, value);
}
