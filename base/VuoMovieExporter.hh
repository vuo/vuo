/**
 * @file
 * VuoMovieExporter interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMOVIEEXPORTER_HH
#define VUOMOVIEEXPORTER_HH

#include <string>
#include <map>
#include "VuoImageBlend.h"
#include "VuoImageRenderer.h"

typedef struct json_object json_object;	///< JSON-C

/**
 * Parameters specifying how an Image Generator composition should be rendered into a movie.
 */
class VuoMovieExporterParameters
{
public:
	/**
	 * Movie export codecs.
	 */
	enum ImageFormat
	{
		H264,
		JPEG,
		ProRes4444,
		ProRes422
	};

	int width;						///< Frame width, in pixels.
	int height;						///< Frame height, in pixels.
	double time;					///< Starting time, in seconds.
	double duration;				///< Duration, in seconds.
	double framerate;				///< Frames per second.
	int spatialSupersample;			///< Resolution multiplier.
	int temporalSupersample;		///< Framerate multiplier.
	float shutterAngle;				///< When using temporal supersampling, specifies the percentage of time the shutter is open (between 0 and 360).
	enum ImageFormat imageFormat;	///< Image format.
	double quality;					///< Image quality (between 0 and 1).
	bool watermark;					///< Should a "Vuo Free Trial" watermark be overlaid on the output images?
	std::map<std::string, json_object *> inputs;	///< Input port names and values.

	static bool isProResAvailable(void);

	VuoMovieExporterParameters(int width = 1024, int height = 768, double time = 0, double duration = 10, double framerate = 30, int spatialSupersample = 1, int temporalSupersample = 1, float shutterAngle = 360, enum ImageFormat imageFormat = H264, double quality = 1, bool watermark = false);
	void print(void);
	double getEstimatedBitrate(void);
	unsigned long getEstimatedSize(void);
};

#if defined(__OBJC__) || defined(DOXYGEN)
@class VuoImageGenerator;
@class AVAssetWriter;
@class AVAssetWriterInput;
@class AVAssetWriterInputPixelBufferAdaptor;
@class NSURL;
@class NSString;
#else
typedef void VuoImageGenerator;
typedef void AVAssetWriter;
typedef void AVAssetWriterInput;
typedef void AVAssetWriterInputPixelBufferAdaptor;
typedef void NSURL;
typedef void NSString;
#endif

/**
 * Runs an Image Generator composition in a separate process, saving each output image to a movie file.
 *
 * \eg{
 * VuoMovieExporter e(…);
 * while (e.exportNextFrame());
 * }
 */
class VuoMovieExporter
{
	VuoMovieExporterParameters parameters;
	unsigned int timebase;
	unsigned int frameMultiplier;
	unsigned int frameCount;
	unsigned int framesExportedSoFar;
	bool firstFrame;
	VuoImageGenerator *generator;
	AVAssetWriter *assetWriter;
	AVAssetWriterInput *assetWriterInput;
	AVAssetWriterInputPixelBufferAdaptor *assetWriterInputPixelBufferAdaptor;
	NSURL *outputMovieUrl;

	VuoImageRenderer resizeImageRenderer;
	VuoGlContext resizeContext;
	VuoShader resizeShader;

	VuoImageBlend blender;

public:
	VuoMovieExporter(std::string compositionFile, std::string outputMovieFile, VuoMovieExporterParameters parameters);
	VuoMovieExporter(std::string compositionString, std::string name, std::string sourcePath, std::string outputMovieFile, VuoMovieExporterParameters parameters);
	~VuoMovieExporter();
	unsigned int getTotalFrameCount(void);
	bool exportNextFrame(void);

private:
	void init(NSString *compositionString, std::string name, std::string sourcePath, std::string outputMovieFile, VuoMovieExporterParameters parameters);
	VuoImage exportNextFramePremium(void);
};

#endif // VUOMOVIEEXPORTER_HH
