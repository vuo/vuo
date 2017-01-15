/**
 * @file
 * VuoSceneGet interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoSceneObject.h"

bool VuoSceneObject_get(VuoText sceneURL, VuoSceneObject *scene, bool center, bool fit, bool hasLeftHandedCoordinates);

#ifdef __cplusplus
}
#endif
