/**
 * @file
 * vuo.scene.combine node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Combine 3D Objects",
					 "description" :
						 "<p>Groups multiple 3D objects together and applies a transformation to the group.</p> \
						 <p>This node is useful for building a 3D scene out of parts. For example, 3D objects that \
						 represent fingers can be grouped to create a hand; when the hand moves, all fingers move with it.</li> \
						 <p><ul> \
						 <li>`transform` — A transform that changes the 3D object's translation, rotation, or scale. \
						 It should use Vuo coordinates.</li> \
						 <li>`childObjects` — The 3D objects to put into a group.</li> \
						 </ul></p> \
						 <p>In Vuo coordinates, (0,0,0) is the center of the scene. \
						 The scene has a width of 2, with x-coordinate -1 on the left edge and 1 on the right edge. \
						 The scene's height is determined by its aspect ratio, with the y-coordinate increasing from bottom to top. \
						 The scene's camera is at (0,0,1), with the z-coordinate increasing from back to front.</p>",
					 "keywords" : [ "scenegraph", "group", "join", "together" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoTransform) transform,
		VuoInputData(VuoList_VuoSceneObject) childObjects,
		VuoOutputData(VuoSceneObject) object
)
{
	*object = VuoSceneObject_make(
				NULL,
				NULL,
				transform,
				childObjects
			);
}
