/**
 * @file
 * example.player.make node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "Player.h"

VuoModuleMetadata({
	"title": "Make Player",
	"description": "Creates a player with an initial score of 0.",
	"keywords": [ "game", "score" ],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ]
	}
});

void nodeEvent
(
	VuoInputData(VuoText) name,
	VuoOutputData(Player) player
)
{
	*player = Player_make(name, 0);
}
