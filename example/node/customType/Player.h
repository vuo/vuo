/**
 * @file
 * Player interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoText.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	VuoText name;
    VuoInteger score;
} Player;

Player Player_make(VuoText name, VuoInteger score);

Player Player_makeFromJson(struct json_object *js);
struct json_object * Player_getJson(const Player value);
char * Player_getSummary(const Player value);

/**
 * Automatically generated function.
 */
///@{
char * Player_getString(const Player value);
void Player_retain(Player value);
void Player_release(Player value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
