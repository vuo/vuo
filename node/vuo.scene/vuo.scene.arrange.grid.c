/**
 * @file
 * vuo.scene.arrange.grid node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Arrange 3D Objects in Grid",
					 "keywords" : [ "align", "place", "position", "lattice", "matrix" ],
					 "version" : "1.0.1",
					 "node": {
						 "exampleCompositions" : [ "AddNoiseToClay.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoSceneObject) objects,
		VuoInputData(VuoBoolean, {"name":"Scale to Fit", "default":true}) scaleToFit,
		VuoInputData(VuoPoint3d, {"default":{"x":0.0,"y":0.0,"z":0.0}, "suggestedMin":{"x":-1.0,"y":-1.0,"z":-1.0}, "suggestedMax":{"x":1.0,"y":1.0,"z":1.0}, "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) center,
		VuoInputData(VuoReal, {"default":1.5, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) width,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":1, "suggestedMax":8}) columns,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) height,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":1, "suggestedMax":8}) rows,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) depth,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":8}) slices,
		VuoOutputData(VuoSceneObject) griddedObject
)
{
	*griddedObject = VuoSceneObject_makeGroup(VuoListCreate_VuoSceneObject(), VuoTransform_makeIdentity());

	unsigned long objectCount = VuoListGetCount_VuoSceneObject(objects);
	unsigned long currentObject = 1;

	VuoReal gridCellWidth = width / columns;
	VuoReal gridCellHeight = height / rows;
	VuoReal gridCellDepth = depth / slices;

	for (unsigned int z = 0; z < slices; ++z)
	{
		VuoReal zPosition = center.z - depth/2 + (depth / slices) * (z + .5);

		for (unsigned int y = 0; y < rows; ++y)
		{
			VuoReal yPosition = center.y + height/2 - (height / rows) * (y + .5);

			for (unsigned int x = 0; x < columns; ++x)
			{
				VuoReal xPosition = center.x - width/2 + (width / columns) * (x + .5);

				VuoSceneObject object = VuoSceneObject_copy(VuoListGetValue_VuoSceneObject(objects, currentObject));

				VuoSceneObject_translate(object, (VuoPoint3d){xPosition, yPosition, zPosition});

				if (scaleToFit)
				{
					VuoBox bounds = VuoSceneObject_bounds(object);
					VuoReal scaleFactor = 1;
					if (bounds.size.x * scaleFactor > gridCellWidth)
						scaleFactor *= gridCellWidth / (bounds.size.x * scaleFactor);
					if (bounds.size.y * scaleFactor > gridCellHeight)
						scaleFactor *= gridCellHeight / (bounds.size.y * scaleFactor);
					if (bounds.size.z * scaleFactor > gridCellDepth)
						scaleFactor *= gridCellDepth / (bounds.size.z * scaleFactor);
					VuoSceneObject_scale(object, (VuoPoint3d){scaleFactor, scaleFactor, scaleFactor});
				}

				VuoListAppendValue_VuoSceneObject(VuoSceneObject_getChildObjects(*griddedObject), object);

				++currentObject;
				if (currentObject > objectCount)
					return;
			}
		}
	}
}
