/**
 * @file
 * Player implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "Player.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "Player",
	"description" : "A participant in a game",
	"version" : "1.0.0"
});
#endif
/// @}

/**
 * Constructs a player from already-registered components.
 */
Player Player_make(VuoText name, VuoInteger score)
{
	Player player;
	player.name = name;
	player.score = score;
	return player;
}

/**
 * Decodes the JSON object @a js to create a new value.
 *
 * @eg{
 *   "name": "Joe Pye Weed",
 *   "score": 100
 * }
 */
Player Player_makeFromJson(json_object *js)
{
	Player value = {"", 0};

	if (json_object_is_type(js, json_type_object))
	{
		json_object *o = NULL;

		if (json_object_object_get_ex(js, "name", &o))
			value.name = VuoText_makeFromJson(o);

		if (json_object_object_get_ex(js, "score", &o))
			value.score = VuoInteger_makeFromJson(o);
	}

	return value;
}

/**
 * Encodes @a value as a JSON object.
 */
json_object * Player_getJson(const Player value)
{
	json_object *js = json_object_new_object();
	json_object_object_add(js, "name", VuoText_getJson(value.name));
	json_object_object_add(js, "score", VuoInteger_getJson(value.score));
	return js;
}

/**
 * Returns a compact string representation of @a value.
 */
char * Player_getSummary(const Player value)
{
	return VuoText_format("%s: %lli", value.name, value.score);
}
