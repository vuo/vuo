/*{
	"DESCRIPTION": "messed up version of v002 erode",
	"CREDIT": "by carter rosenberg",
	"CATEGORIES": [
		"Blur", "v002"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		},
		{
			"NAME": "amount",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 0.1,
			"DEFAULT": 0.01
		}
	]
}*/


varying vec2 texcoord0;
varying vec2 texcoord1;
varying vec2 texcoord2;
varying vec2 texcoord3;
varying vec2 texcoord4;
varying vec2 texcoord5;
varying vec2 texcoord6;
varying vec2 texcoord7;

void main()
{

	vec4 erode = IMG_NORM_PIXEL(inputImage, 0.5 * (texcoord3 + texcoord4));
	
	erode = min(erode, IMG_NORM_PIXEL(inputImage, texcoord0));
	erode = min(erode, IMG_NORM_PIXEL(inputImage, texcoord1));
	erode = min(erode, IMG_NORM_PIXEL(inputImage, texcoord2));
	erode = min(erode, IMG_NORM_PIXEL(inputImage, texcoord3));
	erode = min(erode, IMG_NORM_PIXEL(inputImage, texcoord4));
	erode = min(erode, IMG_NORM_PIXEL(inputImage, texcoord5));
	erode = min(erode, IMG_NORM_PIXEL(inputImage, texcoord6));
	erode = min(erode, IMG_NORM_PIXEL(inputImage, texcoord7));

	gl_FragColor = erode;
}