/*{
	"DESCRIPTION": "Buffers the incoming FFTs for timed display",
	"CREDIT": "by VIDVOX",
	"CATEGORIES": [
		"Generator"
	],
	"INPUTS": [
		{
			"NAME": "fftImage",
			"TYPE": "image"
		},
		{
			"NAME": "clear",
			"TYPE": "event"
		},
		{
			"NAME": "gain",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 5.0,
			"DEFAULT": 1.0
		},
		{
			"NAME": "range",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 1.0
		},
		{
			"NAME": "vertical",
			"TYPE": "bool",
			"DEFAULT": 1.0
		},
		{
			"NAME": "color1",
			"TYPE": "color",
			"DEFAULT": [
				1.0,
				0.125,
				0.25,
				1.0
			]
		},
		{
			"NAME": "color2",
			"TYPE": "color",
			"DEFAULT": [
				0.0,
				0.5,
				1.0,
				1.0
			]
		},
		{
			"NAME": "color3",
			"TYPE": "color",
			"DEFAULT": [
				0.0,
				1.0,
				0.0,
				1.0
			]
		}
	],
	"PERSISTENT_BUFFERS": [
		"fftValues"
	],
	"PASSES": [
		{
			"TARGET":"fftValues",
			"WIDTH:": 256,
			"HEIGHT": 256,
			"DESCRIPTION": "This buffer stores all of the previous fft values"
		},
		{
			
		}
	]
	
}*/


/*

	the lineValues stores the recent values from the incoming values
	on the 1st pass each pixel shifts to the right
	on the 2nd pass go through each pixel, perform a hit test and determine if it needs to be drawn

*/


void main()
{

	//	first pass: read the "fftValues"- remember, we're drawing to the persistent buffer "fftValues" on the first pass
	if (PASSINDEX == 0)	{	
		vec2 loc = vv_FragNormCoord;
		float shift = 1.0/RENDERSIZE.y;
		vec4 p;
		if (loc.y <= shift)	{
			vec4 mixColor = color1;
	
			if (loc.x > 0.5)	{
				mixColor = mix (color2,color3,(loc.x-0.5)*2.0);
			}
			else	{
				mixColor = mix (color1,color2,(loc.x*2.0));
			}
			p = gain * mixColor * IMG_NORM_PIXEL(fftImage, vec2(loc.x * range, 0.5));
		}
		else	{
			loc.y = loc.y - shift;
			p = IMG_NORM_PIXEL(fftValues, loc);
		}
		
		gl_FragColor = (clear) ? vec4(0.0) : p;
	}
	//	second pass: read from "fftValues"
	else if (PASSINDEX == 1)	{
		vec2 loc = vv_FragNormCoord;

		if (vertical)	{
			loc.x = vv_FragNormCoord[1];
			loc.y = vv_FragNormCoord[0];
		}
	
		vec4 fft = IMG_NORM_PIXEL(fftValues,loc);
		
		gl_FragColor = fft;
	}
}
