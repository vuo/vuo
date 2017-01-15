/**
 * @file
 * vuo.kinect.receive node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "libfreenect/libfreenect.h"
#include <OpenGL/CGLMacro.h>
#include <string.h>

VuoModuleMetadata({
					  "title" : "Receive Kinect Images",
					  "keywords" : [ "video", "camera", "infrared", "depth", "sensor", "controller", "motion", "body" ],
					  "version" : "2.0.0",
					  "dependencies" : [
						  "freenect",
						  "usb"
					  ],
					  "node": {
						  "isInterface" : true,
						  "exampleCompositions" : [ "DisplayKinectImages.vuo", "RideRollercoaster.vuo" ]
					  }
				 });

struct nodeInstanceData
{
	bool cancelRequested;
	dispatch_semaphore_t canceled;

	freenect_context *freenectContext;
	freenect_device *freenectDevice;

	void (*receivedImage)(VuoImage);
	void (*receivedDepthImage)(VuoImage);
};

void vuo_kinect_receive_depth_callback(freenect_device *dev, void *v_depth, uint32_t timestamp)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)freenect_get_user(dev);
	freenect_frame_mode mode = freenect_get_current_video_mode(dev);

	uint16_t *depth = (uint16_t*)v_depth;
	float *depthOutput = (float*)malloc(mode.width*mode.height*sizeof(float)*2);

	for(unsigned int y=0; y<mode.height; ++y)
	{
		for(unsigned int x=0; x<mode.width; ++x)
		{
			// Flip the image vertically, because Kinect provides it right-side-up, and VuoImage_makeFromBuffer() expects it flipped.
			unsigned int pos = 2 * ((mode.height-y-1)*mode.width + x);

			uint16_t v = depth[y*mode.width + x];
			if (v)
			{
				depthOutput[pos+0] = v/16383.;
				depthOutput[pos+1] = 1;
			}
			else
			{
				// If the raw depth value is 0, that means the Kinect couldn't figure out the depth, so make that pixel transparent.
				depthOutput[pos+0] = 0;
				depthOutput[pos+1] = 0;
			}
		}
	}

	VuoImage image = VuoImage_makeFromBuffer(depthOutput, GL_LUMINANCE_ALPHA, mode.width, mode.height, VuoImageColorDepth_16, ^(void *buffer){ free(buffer); });
	context->receivedDepthImage(image);
}

void vuo_kinect_receive_rgb_callback(freenect_device *dev, void *rgb, uint32_t timestamp)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)freenect_get_user(dev);
	freenect_frame_mode mode = freenect_get_current_video_mode(dev);

	// Flip the image vertically, because Kinect provides it right-side-up, and VuoImage_makeFromBuffer() expects it flipped.
	int lines = mode.height;
	unsigned int stride = mode.width*3;
	unsigned int size = stride * sizeof(uint8_t);
	void *rgbOut = malloc(size*lines);
	for(int i = 0; i < lines; i++)
		memcpy(rgbOut+stride*(lines-i-1), rgb + stride*i, size);

	VuoImage image = VuoImage_makeFromBuffer(rgbOut, GL_RGB, mode.width, mode.height, VuoImageColorDepth_8, ^(void *buffer){ free(buffer); });
	context->receivedImage(image);
}

void vuo_kinect_receive_worker(void *ctx)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)ctx;

	while (!context->cancelRequested)
	{
		bool haveKinect = false;

		int ret = freenect_init(&context->freenectContext, NULL);
		if (ret == 0)
		{
			freenect_set_log_level(context->freenectContext, FREENECT_LOG_ERROR);

			freenect_select_subdevices(context->freenectContext, (freenect_device_flags)(FREENECT_DEVICE_CAMERA));

			int deviceCount = freenect_num_devices(context->freenectContext);
			if (deviceCount >= 1)
			{
				int deviceNumber = 0;
				// freenect_open_device_by_camera_serial() may also be useful
				int ret = freenect_open_device(context->freenectContext, &context->freenectDevice, deviceNumber);
				if (ret == 0)
					haveKinect = true;
				else
				{
					VUserLog("freenect_open_device(%d) failed, returned %d", deviceNumber, ret);
				}
			}
		}
		else
			VUserLog("freenect_init() failed, returned %d", ret);

		if (haveKinect)
		{
			freenect_set_user(context->freenectDevice, context);

			freenect_set_depth_callback(context->freenectDevice, vuo_kinect_receive_depth_callback);
			freenect_set_video_callback(context->freenectDevice, vuo_kinect_receive_rgb_callback);

			freenect_set_video_mode(context->freenectDevice, freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB));
			freenect_set_depth_mode(context->freenectDevice, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_REGISTERED));

			freenect_start_depth(context->freenectDevice);
			freenect_start_video(context->freenectDevice);

			struct timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = USEC_PER_SEC/10;
			while (!context->cancelRequested)
			{
				int ret = freenect_process_events_timeout(context->freenectContext, &timeout);
				if (ret < 0 && ret != -10)
				{
					VUserLog("freenect_process_events_timeout() failed, returned %d", ret);
							haveKinect = false;
					break;
				}
			}

			if (haveKinect)
			{
				freenect_stop_depth(context->freenectDevice);
				freenect_stop_video(context->freenectDevice);

				freenect_close_device(context->freenectDevice);
			}

			freenect_shutdown(context->freenectContext);
		}

		if (!haveKinect)
			sleep(1);
	}

	if (context->cancelRequested)
		dispatch_semaphore_signal(context->canceled);
}

struct nodeInstanceData * nodeInstanceInit(void)
{
	__block struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->cancelRequested = false;
	context->canceled = dispatch_semaphore_create(0);

	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(receivedImage, VuoImage, {"eventThrottling":"drop"}),
		VuoOutputTrigger(receivedDepthImage, VuoImage, {"eventThrottling":"drop"})
)
{
	(*context)->receivedImage = receivedImage;
	(*context)->receivedDepthImage = receivedDepthImage;

	dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), *context, vuo_kinect_receive_worker);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	(*context)->cancelRequested = true;
	dispatch_semaphore_wait((*context)->canceled, DISPATCH_TIME_FOREVER);

	(*context)->receivedImage = NULL;
	(*context)->receivedDepthImage = NULL;
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	dispatch_release((*context)->canceled);
}
