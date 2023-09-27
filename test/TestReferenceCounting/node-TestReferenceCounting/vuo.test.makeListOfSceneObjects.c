/**
 * @file
 * vuo.test.makeListOfSceneObjects node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Make List of Scene Objects",
					 "description" : "",
					 "version" : "1.0.0",
				 });

void nodeEvent
(
		VuoOutputData(VuoList_VuoSceneObject) sceneObjects
)
{
	VuoSceneObject object = VuoSceneObject_makeEmpty();
	VuoList_VuoSceneObject list = VuoListCreate_VuoSceneObject();
	VuoListAppendValue_VuoSceneObject(list, object);
	*sceneObjects = list;
}
