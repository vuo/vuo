/**
 * @file
 * vuo.shader.make.shadertoy2 node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageRenderer.h"
extern "C" {
#include "VuoTime.h"
}
#include <regex.h>
#include <string>
using namespace std;

VuoModuleMetadata({
	"title": "Make Image with Shadertoy",
	"keywords": [
		"scenegraph", "graphics",
		"opengl", "glsl", "fragment", "pixel",
	],
	"dependencies" : [
		"VuoImageRenderer",
		"VuoTime",
	],
	"version": "3.0.0",
	"node": {
		"exampleCompositions": [ "AnimateConcentricCircles.vuo", "ShowMouseWithShadertoy.vuo" ],
	}
});

static VuoShader makeShader(VuoText fragmentBodyText)
{
	VuoShader s = VuoShader_make("Shadertoy Fragment Shader");
	VuoRetain(s);

	// There isn't a straightforward way to know
	// whether the user-defined shader can produce transparent output,
	// so assume that it can.
	s->isTransparent = true;

	__block bool status = true;
	__block string fragmentBody(fragmentBodyText);
	VuoText_performWithUTF8Locale(^(locale_t locale){
		// Remove shader-wide precision specifiers.
		{
			regex_t re;
			if (regcomp_l(&re, "(precision +)?(lowp|mediump|highp) +(float|int|sampler2D) *;", REG_EXTENDED, locale))
			{
				VUserLog("Error: Couldn't compile regex.");
				status = false;
				return;
			}
			const size_t nmatch = 1;
			regmatch_t pmatch[nmatch];
			while (regexec(&re, fragmentBody.c_str(), nmatch, pmatch, 0) == 0)
				fragmentBody.replace(pmatch[0].rm_so, pmatch[0].rm_eo - pmatch[0].rm_so, "");
		}

		// Rewrite local precision specifiers to just the underlying type.
		{
			regex_t re;
			if (regcomp_l(&re, "(lowp|mediump|highp) +(float|int|sampler2D|vec[234])", REG_EXTENDED, locale))
			{
				VUserLog("Error: Couldn't compile regex.");
				status = false;
				return;
			}
			const size_t nmatch = 3;
			regmatch_t pmatch[nmatch];
			while (regexec(&re, fragmentBody.c_str(), nmatch, pmatch, 0) == 0)
				fragmentBody.replace(pmatch[1].rm_so, pmatch[1].rm_eo - pmatch[1].rm_so, "");
		}

		// Rewrite uint as int.
		{
			regex_t re;
			if (regcomp_l(&re, "[^a-z](u)int[^a-z]", REG_EXTENDED, locale))
			{
				VUserLog("Error: Couldn't compile regex.");
				status = false;
				return;
			}
			const size_t nmatch = 2;
			regmatch_t pmatch[nmatch];
			while (regexec(&re, fragmentBody.c_str(), nmatch, pmatch, 0) == 0)
				fragmentBody.replace(pmatch[1].rm_so, pmatch[1].rm_eo - pmatch[1].rm_so, "");
		}

		// Shadertoy's `main()` sets fragColor.a before passing it to `out vec4 fragColor`,
		// but some GPUs ignore the value passed in to an `out`-only argument.
		// Vuo translates it to `inout`.
		// Many popular Shadertoy shaders rely on this behavior,
		// such as https://www.shadertoy.com/view/4tVBDz
		{
			regex_t re;
			if (regcomp_l(&re, "void[[:space:]]+mainImage[[:space:]]*\\([[:space:]]*(out)[[:space:]]+vec4[[:space:]]+", REG_EXTENDED, locale))
			{
				VUserLog("Error: Couldn't compile regex.");
				status = false;
				return;
			}
			const size_t nmatch = 2;
			regmatch_t pmatch[nmatch];
			while (regexec(&re, fragmentBody.c_str(), nmatch, pmatch, 0) == 0)
				fragmentBody.replace(pmatch[1].rm_so, pmatch[1].rm_eo - pmatch[1].rm_so, "inout");
		}
	});
	if (!status)
		return nullptr;

	string fragmentSource =
		string("#include \"shadertoy-prefix.glsl\"\n")
			 + "#include \"ashima-math.glsl\"\n"
			 + "#include \"ashima-matrix.glsl\"\n"
			 + "#line 0\n"
			 + fragmentBody + "\n"
			 + "#include \"shadertoy-suffix.glsl\"";
	VuoShader_addSource(s, VuoMesh_IndividualTriangles, "#include \"shadertoy.vs\"", nullptr, fragmentSource.c_str());

	return s;
}

struct nodeInstanceData
{
	VuoShader shader;

	VuoPoint2d mousePosition;
	VuoPoint2d mouseClickedPostion;
	bool mouseIsDown;

	unsigned int frameNumber;
	double priorTime;
};

extern "C" struct nodeInstanceData *nodeInstanceInit
(
	VuoInputData(VuoText) fragmentShader
)
{
	auto instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = makeShader(fragmentShader);

	instance->mousePosition = VuoPoint2d_make(0., 0.);
	instance->mouseClickedPostion = VuoPoint2d_make(0., 0.);
	instance->mouseIsDown = false;

	instance->frameNumber = 0;
	instance->priorTime = 0;

	return instance;
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

extern "C" void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoText, {
				"default": "void mainImage(out vec4 fragColor, vec2 fragCoord)\n{\n	fragColor = vec4((fragCoord/iResolution.xy).rgr, 1);\n}",
				"isCodeEditor":true}
			) fragmentShader,
		VuoInputEvent({"eventBlocking":"none","data":"fragmentShader"}) fragmentShaderEvent,
		VuoInputData(VuoReal) time,
		VuoInputData(VuoImage) channel0,
		VuoInputData(VuoImage) channel1,
		VuoInputData(VuoImage) channel2,
		VuoInputData(VuoImage) channel3,
		VuoInputData(VuoInteger, {"default":640, "suggestedMin":1, "suggestedStep":32}) width,
		VuoInputData(VuoInteger, {"default":480, "suggestedMin":1, "suggestedStep":32}) height,
		VuoInputData(VuoImageColorDepth, {"default":"8bpc"}) colorDepth,
		VuoInputData(VuoPoint2d) mousePosition,
		VuoInputData(VuoBoolean, {"name":"Mouse is Pressed"}) mouseIsPressed,
		VuoInputEvent({"eventBlocking":"wall","data":"mousePosition"}) mousePositionEvent,
		VuoInputEvent({"eventBlocking":"wall","data":"mouseIsPressed"}) mouseIsPressedEvent,
		VuoOutputData(VuoImage) shaderImage
)
{
	if (fragmentShader && fragmentShaderEvent)
	{
		if ((*instance)->shader)
			VuoRelease((*instance)->shader);

		(*instance)->shader = makeShader(fragmentShader);
	}

	double timeDelta = time - (*instance)->priorTime;
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "iTime",       time);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "iTimeDelta",  timeDelta);
	VuoShader_setUniform_VuoInteger((*instance)->shader, "iFrame",      (*instance)->frameNumber++);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "iFrameRate",  1./timeDelta);
	(*instance)->priorTime = time;

	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "iResolution", (VuoPoint3d) {(float)width, (float)height, 1.} );

	if (mouseIsPressed)
	{
		(*instance)->mousePosition = convertToPixelCoordinate( mousePosition, VuoPoint2d_make((float)width, (float)height) );

		if (!(*instance)->mouseIsDown)
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

	VuoShader_setUniform_VuoPoint4d( (*instance)->shader, "iMouse", VuoPoint4d_make(
		(*instance)->mousePosition.x,
		(*instance)->mousePosition.y,
		(*instance)->mouseClickedPostion.x,
		(*instance)->mouseClickedPostion.y) );

	if (channel0)
	{
		VuoShader_setUniform_VuoPoint3d((*instance)->shader, "iChannelResolution[0]", VuoPoint3d_make(channel0->pixelsWide, channel0->pixelsHigh, 1.0));
		VuoShader_setUniform_VuoReal   ((*instance)->shader, "iChannelTime[0]",       time);
		VuoShader_setUniform_VuoImage  ((*instance)->shader, "iChannel0",             channel0);
	}

	if (channel1)
	{
		VuoShader_setUniform_VuoPoint3d((*instance)->shader, "iChannelResolution[1]", VuoPoint3d_make(channel1->pixelsWide, channel1->pixelsHigh, 1.0));
		VuoShader_setUniform_VuoReal   ((*instance)->shader, "iChannelTime[1]",       time);
		VuoShader_setUniform_VuoImage  ((*instance)->shader, "iChannel1",             channel1);
	}

	if (channel2)
	{
		VuoShader_setUniform_VuoPoint3d((*instance)->shader, "iChannelResolution[2]", VuoPoint3d_make(channel2->pixelsWide, channel2->pixelsHigh, 1.0));
		VuoShader_setUniform_VuoReal   ((*instance)->shader, "iChannelTime[2]",       time);
		VuoShader_setUniform_VuoImage  ((*instance)->shader, "iChannel2",             channel2);
	}

	if (channel3)
	{
		VuoShader_setUniform_VuoPoint3d((*instance)->shader, "iChannelResolution[3]", VuoPoint3d_make(channel3->pixelsWide, channel3->pixelsHigh, 1.0));
		VuoShader_setUniform_VuoReal   ((*instance)->shader, "iChannelTime[3]",       time);
		VuoShader_setUniform_VuoImage  ((*instance)->shader, "iChannel3",             channel3);
	}

	// Shaders such as https://www.shadertoy.com/view/Xlc3Rf expect iDate.w to include fractional seconds.
	VuoInteger year;
	VuoInteger month;
	VuoInteger dayOfMonth;
	VuoInteger hour;
	VuoInteger minute;
	VuoReal    second;
	if (VuoTime_getComponents(VuoTime_getCurrent(), &year, nullptr, &month, &dayOfMonth, nullptr, nullptr, &hour, &minute, &second))
		VuoShader_setUniform_VuoPoint4d((*instance)->shader, "iDate", VuoPoint4d_make(year, month, dayOfMonth, hour*60*60 + minute*60 + second));

	*shaderImage = VuoImageRenderer_render((*instance)->shader, width, height, colorDepth);
}

extern "C" void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) instance
)
{
	VuoRelease((*instance)->shader);
}
