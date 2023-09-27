/**
 * @file
 * VuoJsonUtilities interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Functions for dealing with JSON.
 */
class VuoJsonUtilities
{
public:
	static string parseString(json_object *o, string key, string defaultString="", bool *foundValue=NULL);
	static string parseObjectString(json_object *o, string outerKey, string innerKey, string defaultString="", bool *foundValue=NULL);
	static int parseInt(json_object *o, string key, int defaultInt=0, bool *foundValue=NULL);
	static bool parseBool(json_object *o, string key, bool defaultBool=false, bool *foundValue=NULL);
	static vector<string> parseArrayOfStrings(json_object *o, string key);
	static map<string, string> parseObjectWithStringValues(json_object *o, string key);

	static json_object *getJson(vector<string> strings);
};
