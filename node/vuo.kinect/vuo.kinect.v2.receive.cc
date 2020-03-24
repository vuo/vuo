/**
 * @file
 * vuo.kinect.v2.receive node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include <iomanip>
#include <sstream>

#include "libfreenect2/libfreenect2.hpp"
#include "libfreenect2/logger.h"

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
	"title": "Receive Kinect v2 Images",
	"keywords": [
		"video", "camera", "infrared", "depth", "sensor", "controller", "motion", "body",
		"Kinect for Xbox One", "Kinect for Windows v2", "K4W2",
	],
	"version": "1.0.0",
	"dependencies": [
		"freenect2",
		"usb",
	],
	"node": {
		"isDeprecated": true,  // Temporary, for testing.
		"exampleCompositions": [ ],
	},
});

class VuoKinect4W2ReceiveListener;

struct nodeInstanceData
{
	VuoKinect4W2ReceiveListener *listener;
	libfreenect2::Freenect2 *freenect;
	libfreenect2::PacketPipeline *pipeline;
	libfreenect2::Freenect2Device *device;

	dispatch_queue_t triggerQueue;  // Ensures the below trigger callbacks don't change between the null check and invocation.
	void (*receivedColorImage)(VuoImage);
	void (*receivedDepthImage)(VuoImage);
	void (*receivedInfraredImage)(VuoImage);
};

class VuoKinect4W2ReceiveLogger : public libfreenect2::Logger
{
	void log(Level level, const std::string &message)
	{
		if (level <= Warning || VuoIsDebugEnabled())
			VUserLog("%s", message.c_str());
	}
};

class VuoKinect4W2ReceiveListener : public libfreenect2::FrameListener
{
public:
	struct nodeInstanceData *context;

	bool onNewFrame(libfreenect2::Frame::Type type, libfreenect2::Frame *frame)
	{
		VDebugLog("timestamp=%5.2f   seq=%4d   %zux%zu   bpp=%zu   type=%d   format=%d   exposure=%5.2f   gain=%5.2f   gamma=%5.2f   status=%d",
			 frame->timestamp / 8000., frame->sequence, frame->width, frame->height, frame->bytes_per_pixel, type, frame->format,
			 frame->exposure, frame->gain, frame->gamma, frame->status);

		if (type == libfreenect2::Frame::Color && type == libfreenect2::Frame::Depth && type == libfreenect2::Frame::Ir)
		{
			VUserLog("Error: Unknown libfreenect2 frame type %d", type);
			return false;
		}

		GLuint glFormat;
		VuoImageColorDepth depth;
		unsigned int bytesPerPixel;
		if (frame->format == libfreenect2::Frame::Float)
		{
			glFormat = GL_LUMINANCE;
			depth = VuoImageColorDepth_32;
			bytesPerPixel = 4;
		}
		else if (frame->format == libfreenect2::Frame::BGRX)
		{
			glFormat = GL_BGR;
			depth = VuoImageColorDepth_8;
			bytesPerPixel = 4;
		}
		else if (frame->format == libfreenect2::Frame::RGBX)
		{
			glFormat = GL_RGB;
			depth = VuoImageColorDepth_8;
			bytesPerPixel = 4;
		}
		else if (frame->format == libfreenect2::Frame::Gray)
		{
			glFormat = GL_LUMINANCE;
			depth = VuoImageColorDepth_8;
			bytesPerPixel = 1;
		}
		else
		{
			VUserLog("Error: Unknown libfreenect2 frame format %d", frame->format);
			return false;
		}

		if (VuoIsDebugEnabled())
		{
			std::ostringstream oss;
			for (int i = 0; i < 32; ++i)
				oss << std::hex << std::setw(2) << std::setfill('0') << (int)frame->data[i];
			VDebugLog("    %s", oss.str().c_str());
		}

		VuoImage image = VuoImage_makeFromBufferWithStride(frame->data, glFormat, frame->width, frame->height, bytesPerPixel * frame->width, depth, ^(void *){
			delete frame;
		});

		dispatch_sync(context->triggerQueue, ^{
			if (type == libfreenect2::Frame::Color && context->receivedColorImage)
				context->receivedColorImage(image);
			else if (type == libfreenect2::Frame::Depth && context->receivedDepthImage)
				context->receivedDepthImage(image);
			else if (type == libfreenect2::Frame::Ir && context->receivedInfraredImage)
				context->receivedInfraredImage(image);
			else
			{
				VuoRetain(image);
				VuoRelease(image);
			}
		});

		// Returning true means we're responsible for deleting the frame.
		return true;
	}
};

extern "C" struct nodeInstanceData *nodeInstanceInit(void)
{
	__block struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	libfreenect2::setGlobalLogger(new VuoKinect4W2ReceiveLogger);

	context->triggerQueue = dispatch_queue_create("vuo.kinect.v2.receive", NULL);

	context->freenect = new libfreenect2::Freenect2;
	if (!context->freenect)
	{
		VUserLog("Error: Couldn't initialize the Freenect2 driver.");
		return context;
	}
	VUserLog("Detected %d Xbox One Kinect device%s.",
		context->freenect->enumerateDevices(),
		context->freenect->enumerateDevices() == 1 ? "" : "s");

	context->pipeline = new libfreenect2::OpenCLPacketPipeline();
	if (!context->pipeline)
	{
		VUserLog("Error: Couldn't initialize libfreenect2::OpenCLPacketPipeline.");
		return context;
	}

#if 1
	context->device = context->freenect->openDefaultDevice(context->pipeline);
#else
	{
		std::vector<std::string> frame_filenames;
		frame_filenames.push_back("1_00000.depth");
		frame_filenames.push_back("2_00001.depth");
		frame_filenames.push_back("3_00002.depth");
		frame_filenames.push_back("4_00003.depth");
		auto fr = new libfreenect2::Freenect2Replay;
		context->device = fr->openDevice(frame_filenames, context->pipeline);
	}
#endif

	if (!context->device)
	{
		VUserLog("Error: Couldn't open the device.");
		return context;
	}

	context->listener = new VuoKinect4W2ReceiveListener;
	context->listener->context = context;

	context->device->setColorFrameListener(context->listener);
	context->device->setIrAndDepthFrameListener(context->listener);

	context->device->start();

	return context;
}

extern "C" void nodeInstanceTriggerStart(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoOutputTrigger(receivedColorImage, VuoImage, {"eventThrottling":"drop"}),
	VuoOutputTrigger(receivedDepthImage, VuoImage, {"eventThrottling":"drop"}),
	VuoOutputTrigger(receivedInfraredImage, VuoImage, {"eventThrottling":"drop"}))
{
	dispatch_sync((*context)->triggerQueue, ^{
		(*context)->receivedColorImage = receivedColorImage;
		(*context)->receivedDepthImage = receivedDepthImage;
		(*context)->receivedInfraredImage = receivedInfraredImage;
	});
}

extern "C" void nodeInstanceEvent(
	VuoInstanceData(struct nodeInstanceData *) context)
{
}

extern "C" void nodeInstanceTriggerStop(
	VuoInstanceData(struct nodeInstanceData *) context)
{
	dispatch_sync((*context)->triggerQueue, ^{
		(*context)->receivedColorImage = nullptr;
		(*context)->receivedDepthImage = nullptr;
		(*context)->receivedInfraredImage = nullptr;
	});
}

extern "C" void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) context)
{
	dispatch_release((*context)->triggerQueue);

	if ((*context)->device)
	{
		(*context)->device->stop();
		(*context)->device->close();
		delete (*context)->device;
	}

	delete (*context)->freenect;
	delete (*context)->listener;
}
