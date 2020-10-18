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

//#define TEST

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

		if (type != libfreenect2::Frame::Color && type != libfreenect2::Frame::Depth && type != libfreenect2::Frame::Ir)
		{
			VUserLog("Error: Unknown libfreenect2 frame type %d", type);
			return false;
		}

		GLuint glFormat;
		VuoImageColorDepth depth;
		unsigned char *pixels = nullptr;
		if (frame->format == libfreenect2::Frame::Float)
		{
			float *frameData = (float *)frame->data;

			if (type == libfreenect2::Frame::Depth)
			{
				glFormat = GL_LUMINANCE_ALPHA;
				depth = VuoImageColorDepth_32;
				float *pixels2 = (float *)malloc(frame->width * frame->height * sizeof(float) * 2);
				for (unsigned int y = 0; y < frame->height; ++y)
					for (unsigned int x = 0; x < frame->width; ++x)
					{
						// Flip the image vertically, because Kinect provides it right-side-up, and OpenGL expects it flipped.
						float f = frameData[y * frame->width + x];
						float a;

						// libfreenect2 says "Non-positive, NaN, and infinity are invalid or missing data".
						if (f <= 0 || isnan(f) || isinf(f))
							f = a = 0;
						else
						{
							// Invert the intensities, because Kinect provides low=near, but people expect high=near.
							// https://b33p.net/kosada/node/15796
							// libfreenect2 says it provides depth data in millimeters;
							// https://pterneas.com/2014/02/08/kinect-for-windows-version-2-overview/ says the range is up to about 4.5m;
							// other sources such as
							// https://social.msdn.microsoft.com/Forums/en-US/c95d3e40-6ed6-47a1-a206-5ff26c889c29/kinect-v2-maximum-range
							// say up to 8m; scale that to 0..1.
							f = 1. - f / 8000;
							a = 1;
						}

						pixels2[(frame->height - y - 1) * frame->width * 2 + x * 2    ] = f;
						pixels2[(frame->height - y - 1) * frame->width * 2 + x * 2 + 1] = a;
					}
				pixels = (unsigned char *)pixels2;
			}
			else if (type == libfreenect2::Frame::Ir)
			{
				glFormat = GL_LUMINANCE;
				depth = VuoImageColorDepth_32;
				float *pixels2 = (float *)malloc(frame->width * frame->height * sizeof(float));
				for (unsigned int y = 0; y < frame->height; ++y)
					for (unsigned int x = 0; x < frame->width; ++x)
						// Flip the image vertically, because Kinect provides it right-side-up, and OpenGL expects it flipped.
						// Scale the Kinect's range 0..65535 to standard OpenGL normalized range.
						pixels2[(frame->height - y - 1) * frame->width + x] = frameData[y * frame->width + x] / 65535;
				pixels = (unsigned char *)pixels2;
			}
			else
			{
				VUserLog("Error: Unknown libfreenect2 float frame type %d", type);
				return false;
			}
		}
		else if (frame->format == libfreenect2::Frame::BGRX
			  || frame->format == libfreenect2::Frame::RGBX)
		{
			glFormat = frame->format == libfreenect2::Frame::BGRX ? GL_BGR : GL_RGB;
			depth = VuoImageColorDepth_8;

			pixels = (unsigned char *)malloc(frame->width * frame->height * 3);
			for (unsigned int y = 0; y < frame->height; ++y)
				for (unsigned int x = 0; x < frame->width; ++x)
				{
					// Flip the image vertically, because Kinect provides it right-side-up, and OpenGL expects it flipped.
					// Pack BGRX/RGBX (4 bytes per pixel) into BGR/RGB (3 bytes per pixel) as OpenGL prefers.
					unsigned char *in  = &frame->data[y * frame->width * 4 + x * 4];
					unsigned char *out = &pixels[(frame->height - y - 1) * frame->width * 3 + x * 3];
					out[0] = in[0];
					out[1] = in[1];
					out[2] = in[2];
				}
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
				oss << std::hex << std::setw(2) << std::setfill('0') << (int)(frame->data[i]) << " ";
			VUserLog("    raw      (char) : %s", oss.str().c_str());

			std::ostringstream oss2;
			for (int i = 0; i < 32; ++i)
				oss2 << ((float *)frame->data)[i] << " ";
			VUserLog("    raw      (float): %s", oss2.str().c_str());

			std::ostringstream oss3;
			for (int i = 0; i < 32; ++i)
				oss3 << std::hex << std::setw(2) << std::setfill('0') << (int)(pixels[i]) << " ";
			VUserLog("    converted (char) : %s", oss3.str().c_str());

			std::ostringstream oss4;
			for (int i = 0; i < 32; ++i)
				oss4 << ((float *)pixels)[i] << " ";
			VUserLog("    converted (float): %s", oss4.str().c_str());
		}

		VuoImage image = VuoImage_makeFromBuffer(pixels, glFormat, frame->width, frame->height, depth, ^(void *){
			free(pixels);
		});
		delete frame;

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

	context->device = context->freenect->openDefaultDevice(context->pipeline);

#ifndef TEST
	if (!context->device)
	{
		VUserLog("Error: Couldn't open the device.");
		return context;
	}
#endif

	context->listener = new VuoKinect4W2ReceiveListener;
	context->listener->context = context;

#ifndef TEST
	context->device->setColorFrameListener(context->listener);
	context->device->setIrAndDepthFrameListener(context->listener);

	context->device->start();
#endif

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
#ifdef TEST
	VuoInputEvent() test,
#endif
	VuoInstanceData(struct nodeInstanceData *) context)
{
#ifdef TEST
	auto depthFrame = new libfreenect2::Frame(512, 424, 4);
	depthFrame->format = libfreenect2::Frame::Float;
	for (int i = 0; i < depthFrame->width * depthFrame->height; ++i)
		((float *)depthFrame->data)[i] = 4000;
	(*context)->listener->onNewFrame(libfreenect2::Frame::Depth, depthFrame);

	auto irFrame = new libfreenect2::Frame(512, 424, 4);
	irFrame->format = libfreenect2::Frame::Float;
	for (int i = 0; i < irFrame->width * irFrame->height; ++i)
		((float *)irFrame->data)[i] = 32767;
	(*context)->listener->onNewFrame(libfreenect2::Frame::Ir, irFrame);

	auto colorFrame = new libfreenect2::Frame(1920, 1080, 4);
	colorFrame->format = libfreenect2::Frame::BGRX;
	for (int i = 0; i < colorFrame->width * colorFrame->height; ++i)
	{
		colorFrame->data[i * 4    ] = 255;
		colorFrame->data[i * 4 + 1] = 0;
		colorFrame->data[i * 4 + 2] = 0;
		colorFrame->data[i * 4 + 3] = 0;
	}
	(*context)->listener->onNewFrame(libfreenect2::Frame::Color, colorFrame);
#endif
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
#ifndef TEST
	if ((*context)->device)
	{
		(*context)->device->stop();
		(*context)->device->close();
	}
#endif

	delete (*context)->listener;
#ifndef TEST
	delete (*context)->device;
#endif
	delete (*context)->freenect;

	dispatch_sync((*context)->triggerQueue, ^{});
	dispatch_release((*context)->triggerQueue);
}
