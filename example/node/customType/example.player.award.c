/**
 * @file
 * example.player.award node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "Player.h"

VuoModuleMetadata({
	"title": "Add to Score",
	"description": "Adds the given number of points to the player's score.",
	"keywords": [ "game" ],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ]
	}
});

void nodeEvent
(
	VuoInputData(Player) player,
	VuoInputData(VuoInteger) points,
	VuoOutputData(Player) awardedPlayer
)
{
	awardedPlayer->name = player.name;
	awardedPlayer->score = player.score + points;
}
