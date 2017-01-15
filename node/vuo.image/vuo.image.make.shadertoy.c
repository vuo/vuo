/**
 * @file
 * vuo.shader.make.shadertoy node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGlContext.h"
#include "VuoImageRenderer.h"
#include <OpenGL/CGLMacro.h>
#include <time.h>

VuoModuleMetadata({
					 "title" : "Make Image with Shadertoy",
					 "keywords" : [ "opengl", "glsl", "scenegraph", "graphics",
						 "lighting", "lit", "lighted",
						 "Blinn", "Phong", "Lambert", "tone", "chroma" ],
					 "version" : "2.1.0",
					 "dependencies" : [
						 "VuoGlContext"
					 ],
					 "node" : {
						  "exampleCompositions" : [ "AnimateConcentricCircles.vuo" ]
					 }
				 });

/**
 *	Shadertoy specific inputs
 *
 *	vec3		iResolution				image			The viewport resolution (z is pixel aspect ratio, usually 1.0)
 *	float		iGlobalTime				image/sound		Current time in seconds
 *	float		iChannelTime[4]			image			Time for channel (if video or sound), in seconds		!!!! NOT SUPPORTED
 *	vec3		iChannelResolution0..3	image/sound		Input texture resolution for each channel
 *	vec4		iMouse					image			xy = current pixel coords (if LMB is down). zw = click pixel
 *	sampler2D	iChannel{i}				image/sound		Sampler for input textures i
 *	vec4		iDate					image/sound		Year, month, day, time in seconds in .xyzw
 *	float		iSampleRate				image/sound		The sound sample rate (typically 44100)	-- In Vuo, const 44100 until AudioSample input is supported.  Only provided so that shaders will successfully compile.
 */

static const char* ShaderHeader = "				\
#version 120								\n 	\
uniform vec3      iResolution;           	\n 	\
uniform float     iGlobalTime;           	\n 	\
uniform vec4      iMouse;					\n 	\
uniform float 	  iChannelTime[4]; 			\n  \
uniform vec3      iChannelResolution[4];	\n 	\
uniform sampler2D iChannel0;	         	\n 	\
uniform sampler2D iChannel1;	         	\n 	\
uniform sampler2D iChannel2;	         	\n 	\
uniform sampler2D iChannel3;	         	\n 	\
uniform vec4 	  iDate;					\n 	\
const float 	  iSampleRate = 44100;      \n  \
#line 0\n";

static const char* shadertoyVertexShader = "#version 120\n 	\
attribute vec4 position;									\
void main()													\
{															\
	gl_Position = position; 								\
}";

struct nodeInstanceData
{
	VuoShader shader;
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;

	VuoPoint2d mousePosition;
	VuoPoint2d mouseClickedPostion;
	bool mouseIsDown;
};

struct nodeInstanceData * nodeInstanceInit(
	VuoInputData(VuoText) fragmentShader
	)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	// Get GL context, ImageRenderer
	instance->glContext = VuoGlContext_use();
	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	instance->shader = VuoShader_make("Shadertoy Fragment Shader");
	VuoRetain(instance->shader);

	if (fragmentShader)
	{
		char *fragmentSource = (char *)malloc(strlen(ShaderHeader) + strlen(fragmentShader) + 1);

		strcpy(fragmentSource, ShaderHeader);
		strcat(fragmentSource, fragmentShader);

		// VLog("\n\n=============================\n\n%s\n\n=============================\n", fragmentSource);

		VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, shadertoyVertexShader, NULL, fragmentSource);

		free(fragmentSource);
	}

	instance->mousePosition = VuoPoint2d_make(0., 0.);
	instance->mouseClickedPostion = VuoPoint2d_make(0., 0.);
	instance->mouseIsDown = false;

	return instance;
}

/**
 *	[wip and unused]
 *  Convert an audio samples array to a 2xSampleCount image, where the first row of pixels represents an FFT in grayscale, and the second
 *  is the normalized PCM data.
 *  https://www.mikeash.com/pyblog/friday-qa-2012-10-26-fourier-transforms-and-ffts.html
 *  http://stackoverflow.com/questions/3398753/using-the-apple-fft-and-accelerate-framework
 */
static VuoImage convertAudioToImage(VuoAudioSamples audio)
{
	int len = audio.sampleCount;

	unsigned char* pixels = (unsigned char*)malloc(sizeof(char)*len*4);
	int n = 0;
	for(int i = 0; i < len; i++)
	{
		pixels[n++] = (unsigned int)(audio.samples[i]*255);
		pixels[n++] = (unsigned int)(audio.samples[i]*255);
		pixels[n++] = (unsigned int)(audio.samples[i]*255);
		pixels[n++] = (unsigned int)(1.);
	}

	VuoImage audioImage = VuoImage_makeFromBuffer(pixels, GL_BGRA, len, 1, VuoImageColorDepth_8, ^(void *buffer){ free(buffer); });

	return audioImage;
}

/**
 *	Converts a Vuo coordinate to a pixel coordinate.
 */
static VuoPoint2d convertToPixelCoordinate(VuoPoint2d point, VuoPoint2d window)
{
	VuoPoint2d normalized;

	normalized.x = (point.x+1)/2.;
	float aspect = window.y / window.x;
	normalized.y = (point.y+aspect) / (aspect*2);

	return VuoPoint2d_scale(normalized, window);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoText, {
				"default": "void main(void)\n{\n	vec2 uv = (gl_FragCoord.xy/iResolution.xy);\n	gl_FragColor = vec4(uv.xyx, 1.);\n}",
				"isCodeEditor":true}
			) fragmentShader,
		VuoInputEvent({"eventBlocking":"none","data":"fragmentShader"}) fragmentShaderEvent,
		VuoInputData(VuoReal) GlobalTime,
		VuoInputData(VuoImage) Channel0,
		VuoInputData(VuoImage) Channel1,
		VuoInputData(VuoImage) Channel2,
		VuoInputData(VuoImage) Channel3,
		VuoInputData(VuoInteger, {"default":640, "suggestedMin":1, "suggestedStep":32}) width,
		VuoInputData(VuoInteger, {"default":480, "suggestedMin":1, "suggestedStep":32}) height,
		VuoInputData(VuoPoint2d) mousePosition,
		VuoInputData(VuoBoolean) mouseIsPressed,
		VuoInputEvent({"eventBlocking":"wall","data":"mousePosition"}) mousePositionEvent,
		VuoInputEvent({"eventBlocking":"wall","data":"mouseIsPressed"}) mouseIsPressedEvent,
		VuoOutputData(VuoImage) shaderImage
)
{
	/**
	 *	If the fragment shader text has changed, reload the shader.
	 */
	if (fragmentShader && fragmentShaderEvent)
	{
		if((*instance)->shader)
		{
			VuoRelease((*instance)->shader);
		}

		(*instance)->shader = VuoShader_make("Shadertoy Fragment Shader");
		VuoRetain((*instance)->shader);

		char* fragmentSource = (char*)malloc(strlen(ShaderHeader) + strlen(fragmentShader) + 1);

		strcpy(fragmentSource, ShaderHeader);
		strcat(fragmentSource, fragmentShader);

		VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, shadertoyVertexShader, NULL, fragmentSource);

		free(fragmentSource);
	}

	/**
	 *	Time
	 */
	VuoShader_setUniform_VuoReal((*instance)->shader, "iGlobalTime", GlobalTime);

	/**
	 *	Resolution
	 */
	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "iResolution", (VuoPoint3d) {(float)width, (float)height, 1.} );

	if(mouseIsPressed)
	{
		(*instance)->mousePosition = convertToPixelCoordinate( mousePosition, VuoPoint2d_make((float)width, (float)height) );

		if(!(*instance)->mouseIsDown)
		{
			(*instance)->mouseIsDown = true;
			(*instance)->mouseClickedPostion = (*instance)->mousePosition;
		}
	}
	else
	{
		(*instance)->mouseClickedPostion = (VuoPoint2d) {-420, -285};	// true story, these are the actual coordinates shadertoy uses
		(*instance)->mouseIsDown = false;
	}

	/**
	 *	Mouse Coordinates / Clicked
	 */
	VuoShader_setUniform_VuoPoint4d( (*instance)->shader, "iMouse", VuoPoint4d_make(
		(*instance)->mousePosition.x,
		(*instance)->mousePosition.y,
		(*instance)->mouseClickedPostion.x,
		(*instance)->mouseClickedPostion.y) );

	/**
	 *	Channels (Textures and Audio ...eventually)
	 */
	if(Channel0 != NULL)
	{
		VuoShader_setUniform_VuoPoint3d((*instance)->shader, "iChannelResolution[0]", VuoPoint3d_make(Channel0->pixelsWide, Channel0->pixelsHigh, 1.0));
		VuoShader_setUniform_VuoImage((*instance)->shader, "iChannel0", Channel0);
	}

	if(Channel1 != NULL)
	{
		VuoShader_setUniform_VuoPoint3d((*instance)->shader, "iChannelResolution[1]", VuoPoint3d_make(Channel1->pixelsWide, Channel1->pixelsHigh, 1.0));
		VuoShader_setUniform_VuoImage((*instance)->shader, "iChannel1", Channel1);
	}

	if(Channel2 != NULL)
	{
		VuoShader_setUniform_VuoPoint3d((*instance)->shader, "iChannelResolution[2]", VuoPoint3d_make(Channel2->pixelsWide, Channel2->pixelsHigh, 1.0));
		VuoShader_setUniform_VuoImage((*instance)->shader, "iChannel2", Channel2);
	}

	if(Channel3 != NULL)
	{
		VuoShader_setUniform_VuoPoint3d((*instance)->shader, "iChannelResolution[3]", VuoPoint3d_make(Channel3->pixelsWide, Channel3->pixelsHigh, 1.0));
		VuoShader_setUniform_VuoImage((*instance)->shader, "iChannel3", Channel3);
	}

	/**
	 *	Date and time.
	 * 	http://stackoverflow.com/questions/1442116/how-to-get-date-and-time-value-in-c-program
	 */
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	VuoShader_setUniform_VuoPoint4d((*instance)->shader, "iDate", (VuoPoint4d) { tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, (tm.tm_hour * 60 * 60) + tm.tm_sec } );

	*shaderImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, width, height, VuoImageColorDepth_8);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
