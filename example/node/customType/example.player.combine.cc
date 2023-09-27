/**
 * @file
 * example.player.combine node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "Player.h"
#include "VuoList_Player.h"
#include <numeric>

extern "C" {
VuoModuleMetadata({
	"title": "Combine Scores",
	"description": "Combines the scores from multiple players to calculate a team score.",
	"keywords": [ "game" ],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ "ShowTeamScore.vuo" ]
	}
});
}

extern "C" void nodeEvent
(
	VuoInputData(VuoList_Player) players,
	VuoInputData(VuoInteger, {"menuItems":[
		{"value":0, "name":"Total"},
		{"value":1, "name":"Maximum"},
	], "default":0}) method,
	VuoOutputData(VuoInteger) combinedScore
)
{
	if (VuoListGetCount_Player(players) == 0)
	{
		*combinedScore = 0;
		return;
	}

	__block std::vector<VuoInteger> scores;
	VuoListForeach_Player(players, ^(const Player player)
	{
		scores.push_back(player.score);
		return true;
	});

	if (method == 0)  // total
	{
		*combinedScore = std::accumulate(scores.begin(), scores.end(), (VuoInteger)0);
	}
	else  // max
	{
		auto iter = std::max_element(scores.begin(), scores.end());
		*combinedScore = *iter;
	}
}
