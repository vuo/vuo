/**
 * @file
 * vuo.image.rotate node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGlContext.h"
#include "VuoSceneRenderer.h"
#include "VuoLayer.h"
#include "VuoRenderedLayers.h"

VuoModuleMetadata({
					 "title" : "Rotate Image",
					 "keywords" : [ "spin", "90", "180", "flip" ],
					 "version" : "1.0.1",
					 "dependencies" : [
						 "VuoLayer",
						 "VuoGlContext",
						 "VuoSceneRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "RotateMovie.vuo" ]
					 }
				 });

const float DEG_2_RAD = 0.0174532925;

struct nodeInstanceData
{
	VuoGlContext glContext;
	VuoSceneRenderer *sceneRenderer;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));

	context->glContext = VuoGlContext_use();

	context->sceneRenderer = VuoSceneRenderer_make(context->glContext, 1);
	VuoRetain(context->sceneRenderer);

	VuoRegister(context, free);
	return context;
}

void getRotatedImageDimensions( int width, int height, float rotation_radians, int *resized_width, int *resized_height )
{
	VuoPoint2d corners[3] =
	{
		(VuoPoint2d){width,0},
		(VuoPoint2d){width,height},
		(VuoPoint2d){0,height}
	};

	float x_min = 0., x_max = 0., y_min = 0., y_max = 0.;
	for(int i = 0; i < 3; i++)
	{
		float ru = (cos(rotation_radians) * corners[i].x + sin(rotation_radians) * corners[i].y);
		float rv = (cos(rotation_radians) * corners[i].y - sin(rotation_radians) * corners[i].x);

		x_min = fmin( ru, x_min );
		y_min = fmin( rv, y_min );

		x_max = fmax( ru, x_max );
		y_max = fmax( rv, y_max );
	}

	*resized_width = x_max - x_min;
	*resized_height = y_max - y_min;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"suggestedMin":-180.0, "suggestedMax":180.0, "suggestedStep":15.0}) rotation,
		VuoInputData(VuoBoolean, { "default":false } ) expandBounds,
		VuoOutputData(VuoImage) rotatedImage
)
{
	if(!image) return;

	int width = image->pixelsWide;
	int height = image->pixelsHigh;
	float aspect = 2.;

	if(expandBounds)
	{
		getRotatedImageDimensions(image->pixelsWide, image->pixelsHigh, rotation * DEG_2_RAD, &width, &height);
		aspect = fmax( (image->pixelsWide/(float)width), (image->pixelsHigh/(float)height) ) * 2;
	}

	VuoSceneObject rootSceneObject = VuoLayer_make(VuoText_make("Rotated Image"), image, (VuoPoint2d){0,0}, rotation, aspect, 1.).sceneObject;

	VuoSceneRenderer_setRootSceneObject((*context)->sceneRenderer, rootSceneObject);
	VuoSceneRenderer_regenerateProjectionMatrix((*context)->sceneRenderer, width, height);

	VuoSceneRenderer_renderToImage((*context)->sceneRenderer, rotatedImage, VuoImage_getColorDepth(image), VuoMultisample_4, NULL);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->sceneRenderer);
	VuoGlContext_disuse((*context)->glContext);
}
