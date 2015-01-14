/**
 * @file
 * vuo.movie.decodeImage node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMovie.h"

VuoModuleMetadata({
					  "title" : "Decode Movie Image",
					  "keywords" : [
						  "animation",
						  "avi",
						  "dv", "dvc",
						  "h264", "h.264",
						  "mjpeg",
						  "mpeg", "m4v", "mp4",
						  "quicktime", "qt", "aic", "prores",
						  "video",
					  ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : true,
						  "exampleCompositions" : [ ]
					  },
					  "dependencies" : [
						"VuoMovie"
					  ]
				  });


struct nodeInstanceData
{
	int blah; /// @todo
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	/// @todo

	return context;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoText) movieURL,
		VuoInputEvent(VuoPortEventBlocking_Wall, movieURL) movieURLEvent,
		VuoInputData(VuoReal, {"suggestedMin":0.}) frameTime,
//		VuoInputData(VuoLoopType) loop /// @todo
		VuoOutputData(VuoImage) image
)
{
	if (movieURLEvent)
	{
		/// @todo switch to another movie
	}

	/// @todo decode frame
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	/// @todo
}
