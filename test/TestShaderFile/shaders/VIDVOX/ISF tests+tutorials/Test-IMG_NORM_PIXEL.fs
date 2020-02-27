/*{
	"DESCRIPTION": "demonstrates the use of IMG_NORM_PIXEL to fetch a pixel color from an input",
	"CREDIT": "by zoidberg",
	"CATEGORIES": [
		"TEST-GLSL FX"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		}
	]
}*/

void main()
{
	vec4		test = IMG_NORM_PIXEL(inputImage, vv_FragNormCoord);
	gl_FragColor = test;
}
