/*{
	"DESCRIPTION": "Does a fast faked data-mosh style",
	"CREDIT": "by VIDVOX",
	"CATEGORIES": [
		"Glitch"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		},
		{
			"NAME": "update_keyframe",
			"TYPE": "bool",
			"DEFAULT": 1.0
		},
		{
			"NAME": "update_rate",
			"TYPE": "float",
			"MIN": 0.01,
			"MAX": 1.0,
			"DEFAULT": 0.95
		},
		{
			"NAME": "sharpen",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 10.0,
			"DEFAULT": 1.0
		},
		{
			"NAME": "blur",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 2.0,
			"DEFAULT": 0.25
		},
		{
			"NAME": "posterize",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 0.25
		},
		{
			"NAME": "mode",
			"VALUES": [
				0,
				1,
				2
			],
			"LABELS": [
				"relative",
				"absolute",
				"difference"
			],
			"DEFAULT": 0,
			"TYPE": "long"
		}
	],
	"PERSISTENT_BUFFERS": {
		"keyFrameBuffer1": {
			"WIDTH": "$WIDTH/16.0",
			"HEIGHT": "$HEIGHT/16.0"
		},
		"keyFrameBuffer2": {
			"WIDTH": "$WIDTH",
			"HEIGHT": "$HEIGHT"
		}
	},
	"PASSES": [
		{
			"TARGET":"keyFrameBuffer1"
		},
		{
			"TARGET":"keyFrameBuffer2"
		}
	]
	
}*/

varying vec2 left_coord;
varying vec2 right_coord;
varying vec2 above_coord;
varying vec2 below_coord;

varying vec2 lefta_coord;
varying vec2 righta_coord;
varying vec2 leftb_coord;
varying vec2 rightb_coord;

void main()
{

	//	first pass: read the "inputImage"- remember, we're drawing to the persistent buffer "keyFrameBuffer1" on the first pass
	//	store it if we're taking in a new keyframe
	if (PASSINDEX == 0)	{
		vec4		stalePixel = IMG_THIS_NORM_PIXEL(keyFrameBuffer1);		
		vec4		freshPixel = IMG_THIS_NORM_PIXEL(inputImage);
		if (update_keyframe==true)	{
			gl_FragColor = freshPixel;
		}
		else	{
			gl_FragColor = stalePixel;
		}
	}
	//	second pass: read from "bufferVariableNameA".  output looks chunky and low-res.
	else if (PASSINDEX == 1)	{
		vec4		stalePixel1 = IMG_THIS_NORM_PIXEL(keyFrameBuffer1);
		vec4		stalePixel2 = IMG_THIS_NORM_PIXEL(keyFrameBuffer2);
		vec4 color = IMG_THIS_NORM_PIXEL(inputImage);
		vec4 colorL = IMG_NORM_PIXEL(inputImage, left_coord);
		vec4 colorR = IMG_NORM_PIXEL(inputImage, right_coord);
		vec4 colorA = IMG_NORM_PIXEL(inputImage, above_coord);
		vec4 colorB = IMG_NORM_PIXEL(inputImage, below_coord);

		vec4 colorLA = IMG_NORM_PIXEL(inputImage, lefta_coord);
		vec4 colorRA = IMG_NORM_PIXEL(inputImage, righta_coord);
		vec4 colorLB = IMG_NORM_PIXEL(inputImage, leftb_coord);
		vec4 colorRB = IMG_NORM_PIXEL(inputImage, rightb_coord);

		vec4 final = color;
		vec4 diff = (color - stalePixel2);
		if (blur > 0.0)	{
			float blurAmount = blur / 8.0;
			diff = diff * (1.0 - blur) + (colorL + colorR + colorA + colorB + colorLA + colorRA + colorLB + colorRB) * blurAmount;
		}
		
		if (mode == 0)	{
			final = mix(stalePixel2,(diff + stalePixel1),update_rate);
		}
		else if (mode == 1)	{
			final = mix(stalePixel2,abs(diff) + stalePixel1,update_rate);
		}
		else if (mode == 2)	{
			final = mix(stalePixel2,abs(diff + stalePixel2) * stalePixel1,update_rate);
		}
		
		if (posterize > 0.0)	{
			float qLevel = 128.0 - posterize * 126.0;
			final = floor(final*qLevel)/qLevel;
		}
		
		if (sharpen > 0.0)	{
			final = final + sharpen * (8.0*color - colorL - colorR - colorA - colorB - colorLA - colorRA - colorLB - colorRB);
		}
		
		gl_FragColor = final;
	}
}
