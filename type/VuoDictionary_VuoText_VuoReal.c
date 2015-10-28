/**
 * @file
 * VuoDictionary_VuoText_VuoReal implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <string.h>

#include "type.h"
#include "VuoDictionary_VuoText_VuoReal.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Dictionary of Text keys and Real values",
					  "description" : "",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoReal",
						"VuoText",
						"VuoList_VuoReal",
						"VuoList_VuoText"
					  ]
				  });
#endif
/// @}

/**
 * @ingroup VuoDictionary_VuoText_VuoReal
 * Decodes the JSON object to create a new value.
 *
 * @eg{
 *   {
 *     "keys" : ["a","b","c"],
 *     "values" : [1.2, 3.4, 5.6]
 *   }
 * }
 */
VuoDictionary_VuoText_VuoReal VuoDictionary_VuoText_VuoReal_valueFromJson(json_object *js)
{
	VuoDictionary_VuoText_VuoReal d;
	json_object *o = NULL;

	bool hasKeys = json_object_object_get_ex(js, "keys", &o);
	d.keys = VuoList_VuoText_valueFromJson(hasKeys ? o : NULL);

	bool hasValues = json_object_object_get_ex(js, "values", &o);
	d.values = VuoList_VuoReal_valueFromJson(hasValues ? o : NULL);

	return d;
}

/**
 * @ingroup VuoDictionary_VuoText_VuoReal
 * Encodes the value as a JSON object.
 */
json_object * VuoDictionary_VuoText_VuoReal_jsonFromValue(const VuoDictionary_VuoText_VuoReal d)
{
	json_object *js = json_object_new_object();

	json_object_object_add(js, "keys", VuoList_VuoText_jsonFromValue(d.keys));
	json_object_object_add(js, "values", VuoList_VuoReal_jsonFromValue(d.values));

	return js;
}

/**
 * @ingroup VuoDictionary_VuoText_VuoReal
 * Returns a brief description of the value.
 *
 * (When this type is replaced with a generic VuoDictionary type, this function will be replaced with
 * a nicer implementation, more like VuoList_summaryFromValue().)
 */
char * VuoDictionary_VuoText_VuoReal_summaryFromValue(const VuoDictionary_VuoText_VuoReal d)
{
	char *keysSummary = VuoList_VuoText_summaryFromValue(d.keys);
	char *valuesSummary = VuoList_VuoReal_summaryFromValue(d.values);

	char *summary = VuoText_format("<ul><li>Keys: %s</li><li>Values: %s</li></ul>", keysSummary, valuesSummary);

	free(keysSummary);
	free(valuesSummary);

	return summary;
}

/**
 * @ingroup VuoDictionary_VuoText_VuoReal
 * Creates an empty dictionary.
 */
VuoDictionary_VuoText_VuoReal VuoDictionaryCreate_VuoText_VuoReal(void)
{
	VuoDictionary_VuoText_VuoReal d = { VuoListCreate_VuoText(), VuoListCreate_VuoReal() };
	return d;
}

/**
 * @ingroup VuoDictionary_VuoText_VuoReal
 * Returns the keys of the dictionary.
 */
VuoList_VuoText VuoDictionaryGetKeys_VuoText_VuoReal(VuoDictionary_VuoText_VuoReal d)
{
	VuoList_VuoText keysCopy = VuoListCreate_VuoText();

	unsigned long count = VuoListGetCount_VuoText(d.keys);
	for (unsigned long i = 1; i <= count; ++i)
	{
		VuoText key = VuoListGetValue_VuoText(d.keys, i);
		VuoListAppendValue_VuoText(keysCopy, key);
	}

	return keysCopy;
}

/**
 * @ingroup VuoDictionary_VuoText_VuoReal
 * Returns the value mapped from @a key in the dictionary.
 */
VuoReal VuoDictionaryGetValueForKey_VuoText_VuoReal(VuoDictionary_VuoText_VuoReal d, VuoText key)
{
	unsigned long count = VuoListGetCount_VuoText(d.keys);
	for (unsigned long i = 1; i <= count; ++i)
		if (VuoText_areEqual(key, VuoListGetValue_VuoText(d.keys, i)))
			return VuoListGetValue_VuoReal(d.values, i);

	return 0;
}

/**
 * @ingroup VuoDictionary_VuoText_VuoReal
 * Sets the value mapped from @a key in the dictionary to @a value.
 *
 * (Currently, this only allows adding a key-value mapping, not changing an existing one. When this type
 * is replaced with a generic VuoDictionary type, this function will allow the latter.)
 */
void VuoDictionarySetKeyValue_VuoText_VuoReal(VuoDictionary_VuoText_VuoReal d, VuoText key, VuoReal value)
{
	VuoListAppendValue_VuoText(d.keys, key);
	VuoListAppendValue_VuoReal(d.values, value);
}
