/**
 * @file
 * VuoJsonUtilities implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoJsonUtilities.hh"

/**
 * Parses the string value for `key` in the top level of the JSON object.
 *
 * If no such value is found, returns `defaultString`.
 */
string VuoJsonUtilities::parseString(json_object *o, string key, string defaultString, bool *foundValue)
{
	string s = defaultString;
	if (foundValue)
		*foundValue = false;

	json_object *stringObject = NULL;
	if (json_object_object_get_ex(o, key.c_str(), &stringObject))
	{
		if (json_object_get_type(stringObject) == json_type_string)
		{
			s = json_object_get_string(stringObject);
			if (foundValue)
				*foundValue = true;
		}
	}

	return s;
}

/**
 * Parses the string value for path `outerKey/innerKey` of the JSON object.
 *
 * If no such value is found, returns `defaultString`.
 */
string VuoJsonUtilities::parseObjectString(json_object *o, string outerKey, string innerKey, string defaultString, bool *foundValue)
{
	string s = defaultString;
	if (foundValue)
		*foundValue = false;

	json_object *outerObject = NULL;
	if (json_object_object_get_ex(o, outerKey.c_str(), &outerObject))
	{
		json_object *innerObject = NULL;
		if (json_object_object_get_ex(outerObject, innerKey.c_str(), &innerObject))
		{
			if (json_object_get_type(innerObject) == json_type_string)
			{
				s = json_object_get_string(innerObject);
				if (foundValue)
					*foundValue = true;
			}
		}
	}

	return s;
}

/**
 * Parses the integer value for `key` in the top level of the JSON object.
 *
 * If no such value is found, returns `defaultInt`.
 */
int VuoJsonUtilities::parseInt(json_object *o, string key, int defaultInt, bool *foundValue)
{
	int i = defaultInt;
	if (foundValue)
		*foundValue = false;

	json_object *intObject = NULL;
	if (json_object_object_get_ex(o, key.c_str(), &intObject))
	{
		if (json_object_get_type(intObject) == json_type_int)
		{
			i = json_object_get_int(intObject);
			if (foundValue)
				*foundValue = true;
		}
	}

	return i;
}

/**
 * Parses the boolean value for `key` in the top level of the JSON object.
 *
 * If no such value is found, returns `defaultBool`.
 */
bool VuoJsonUtilities::parseBool(json_object *o, string key, bool defaultBool, bool *foundValue)
{
	bool b = defaultBool;
	if (foundValue)
		*foundValue = false;

	json_object *boolObject = NULL;
	if (json_object_object_get_ex(o, key.c_str(), &boolObject))
	{
		if (json_object_get_type(boolObject) == json_type_boolean)
		{
			b = json_object_get_boolean(boolObject);
			if (foundValue)
				*foundValue = true;
		}
	}

	return b;
}

/**
 * Parses the array-of-strings value for `key` in the top level of the JSON object.
 *
 * If no such value is found, returns an empty vector.
 */
vector<string> VuoJsonUtilities::parseArrayOfStrings(json_object *o, string key)
{
	vector<string> items;
	json_object *arrayObject = NULL;
	if (json_object_object_get_ex(o, key.c_str(), &arrayObject))
	{
		if (json_object_get_type(arrayObject) == json_type_array)
		{
			int itemCount = json_object_array_length(arrayObject);
			for (int i = 0; i < itemCount; ++i)
			{
				json_object *item = json_object_array_get_idx(arrayObject, i);
				if (json_object_get_type(item) == json_type_string)
					items.push_back( json_object_get_string(item) );
			}
		}
	}
	return items;
}

/**
 * Converts a vector of std::strings into a JSON array of strings.
 */
json_object *VuoJsonUtilities::getJson(vector<string> strings)
{
	json_object *a = json_object_new_array();
	for (vector<string>::iterator i = strings.begin(); i != strings.end(); ++i)
		json_object_array_add(a, json_object_new_string(i->c_str()));
	return a;
}
