/*{
	"DESCRIPTION": "demonstrates the use of multiple image-type inputs",
	"CREDIT": "by zoidberg",
	"CATEGORIES": [
		"TEST-GLSL FX"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		}
	],
	"IMPORTED": {
		"blendImage": {
			"PATH": "Hexagon.tiff"
		}
	}
}*/

void main()
{
	vec4		srcPixel = IMG_NORM_PIXEL(inputImage, vv_FragNormCoord);
	vec4		blendPixel = IMG_NORM_PIXEL(blendImage, vv_FragNormCoord);
	
	gl_FragColor = (srcPixel + blendPixel)/2.0;
}
