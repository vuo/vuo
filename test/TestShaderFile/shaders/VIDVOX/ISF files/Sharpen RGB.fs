/*{
	"CREDIT": "by VIDVOX",
	"CATEGORIES": [
		"Sharpen"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		},
		{
			"NAME": "intensityR",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 10.0,
			"DEFAULT": 1.0
		},
		{
			"NAME": "intensityG",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 10.0,
			"DEFAULT": 1.0
		},
		{
			"NAME": "intensityB",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 10.0,
			"DEFAULT": 1.0
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

float gray(vec4 n)
{
	return (n.r + n.g + n.b)/3.0;
}

void main()
{

	vec4 color = IMG_THIS_PIXEL(inputImage);
	vec4 colorL = IMG_NORM_PIXEL(inputImage, left_coord);
	vec4 colorR = IMG_NORM_PIXEL(inputImage, right_coord);
	vec4 colorA = IMG_NORM_PIXEL(inputImage, above_coord);
	vec4 colorB = IMG_NORM_PIXEL(inputImage, below_coord);

	vec4 colorLA = IMG_NORM_PIXEL(inputImage, lefta_coord);
	vec4 colorRA = IMG_NORM_PIXEL(inputImage, righta_coord);
	vec4 colorLB = IMG_NORM_PIXEL(inputImage, leftb_coord);
	vec4 colorRB = IMG_NORM_PIXEL(inputImage, rightb_coord);

	vec4 final = color;
	final.r = color.r + intensityR * (8.0*color.r - colorL.r - colorR.r - colorA.r - colorB.r - colorLA.r - colorRA.r - colorLB.r - colorRB.r);
	final.g = color.g + intensityG * (8.0*color.g - colorL.g - colorR.g - colorA.g - colorB.g - colorLA.g - colorRA.g - colorLB.g - colorRB.g);
	final.b = color.b + intensityB * (8.0*color.b - colorL.b - colorR.b - colorA.b - colorB.b - colorLA.b - colorRA.b - colorLB.b - colorRB.b);
	
	gl_FragColor = final;
}