/**
 * @file
 * vuo.image.color.cga node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Make CGA Image",
					 "keywords" : [
						 "colorspace",
						 "retro", "8-bit", "IBM PC", "1980s", "color graphics adapter",
						 "banding", "raster", "gif", "quantize", "reduce",
						 "contrast",
						 "filter",
					 ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoImageRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)

	// Inputs
	uniform sampler2D texture;
	uniform vec4 color0;
	uniform vec4 color1;
	uniform vec4 color2;
	uniform vec4 color3;
	varying vec4 fragmentTextureCoordinate;

	float dist2(vec3 a, vec3 b)
	{
		float x = a.x - b.x;
		float y = a.y - b.y;
		float z = a.z - b.z;
		return x*x + y*y + z*z;
	}

	void main(void)
	{
		vec4 color = VuoGlsl_sample(texture, fragmentTextureCoordinate.xy);
		color.rgb /= color.a;

		float d0 = dist2(color.rgb, color0.rgb);
		float d1 = dist2(color.rgb, color1.rgb);
		float d2 = dist2(color.rgb, color2.rgb);
		float d3 = dist2(color.rgb, color3.rgb);

		float closestColorDistance = min(min(min(d0, d1), d2), d3);
		vec3 chosenColor;
		if (closestColorDistance == d0)
			chosenColor = color0.rgb;
		else if (closestColorDistance == d1)
			chosenColor = color1.rgb;
		else if (closestColorDistance == d2)
			chosenColor = color2.rgb;
		else // if (closestColorDistance == d3)
			chosenColor = color3.rgb;

		gl_FragColor = vec4(chosenColor * color.a, color.a);
	}
);

struct nodeInstanceData
{
	VuoShader shader;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_make("Map to CGA Colors");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) instance,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoInteger, {"menuItems":{
		"0":"Black",
		"1":"Blue",
		"2":"Green",
		"3":"Cyan",
		"4":"Red",
		"5":"Magenta",
		"6":"Brown",
		"7":"Light gray",
		"8":"Gray",
		"9":"Light blue",
		"10":"Light green",
		"11":"Light cyan",
		"12":"Light red",
		"13":"Light magenta",
		"14":"Yellow",
		"15":"White"
	},"default":0}) backgroundColor,
	VuoInputData(VuoInteger, {"menuItems":{
		"0":"Cyan, Magenta, Gray (Dark)",
		"1":"Cyan, Magenta, White (Light)",
		"2":"Green, Red, Brown (Dark)",
		"3":"Green, Red, Yellow (Light)",
		"4":"Cyan, Red, Gray (Dark)",
		"5":"Cyan, Red, White (Light)"
	},"default":1}) palette,
	VuoOutputData(VuoImage, {"name":"CGA Image"}) cgaImage
)
{
	if (! image)
	{
		*cgaImage = NULL;
		return;
	}

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);

	char colorNumbers[4];
	if (palette == 0)
	{
		colorNumbers[1] = 3;
		colorNumbers[2] = 5;
		colorNumbers[3] = 7;
	}
	else if (palette == 1)
	{
		colorNumbers[1] = 11;
		colorNumbers[2] = 13;
		colorNumbers[3] = 15;
	}
	else if (palette == 2)
	{
		colorNumbers[1] = 2;
		colorNumbers[2] = 4;
		colorNumbers[3] = 6;
	}
	else if (palette == 3)
	{
		colorNumbers[1] = 10;
		colorNumbers[2] = 12;
		colorNumbers[3] = 14;
	} else if (palette == 4)
	{
		colorNumbers[1] = 3;
		colorNumbers[2] = 4;
		colorNumbers[3] = 7;
	}
	else // if (palette == 5)
	{
		colorNumbers[1] = 11;
		colorNumbers[2] = 12;
		colorNumbers[3] = 15;
	}
	colorNumbers[0] = VuoInteger_clamp(backgroundColor, 0, 15);

	VuoColor colors[4];
	for (int i = 0; i < 4; ++i)
	{
		int c = colorNumbers[i];
		float r = ((c & 4) >> 2) * 2./3. + ((c & 8) >> 3) / 3.;
		float g = ((c & 2) >> 1) * 2./3. + ((c & 8) >> 3) / 3.;
		float b =  (c & 1)       * 2./3. + ((c & 8) >> 3) / 3.;

		if (c == 6) // dark yellow -> brown
			g /= 2;

		colors[i] = VuoColor_makeWithRGBA(r,g,b,1);
	}

	VuoShader_setUniform_VuoColor((*instance)->shader, "color0", colors[0]);
	VuoShader_setUniform_VuoColor((*instance)->shader, "color1", colors[1]);
	VuoShader_setUniform_VuoColor((*instance)->shader, "color2", colors[2]);
	VuoShader_setUniform_VuoColor((*instance)->shader, "color3", colors[3]);

	// Render.
	*cgaImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
