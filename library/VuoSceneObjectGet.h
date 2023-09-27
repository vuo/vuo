/**
 * @file
 * VuoSceneGet interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoSceneObject.h"

bool VuoSceneObject_get(VuoText sceneURL, VuoSceneObject *scene, bool center, bool fit, bool hasLeftHandedCoordinates) VuoWarnUnusedResult;

#ifdef __cplusplus
}
#endif
