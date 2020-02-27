/*{
	"CREDIT": "by zoidberg",
	"CATEGORIES": [
		"Color Effect"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		},
		{
			"NAME": "levels",
			"TYPE": "float",
			"MIN": 2.0,
			"MAX": 30.0,
			"DEFAULT": 30.0
		}
	]
}*/

void main() {
	//	get the src pixel, convert to HSL, posterize the 'L', convert back to RGB
	vec4		srcPixel = IMG_THIS_PIXEL(inputImage);
	vec4		amountPerLevel = vec4(1.0/levels);
	vec4		numOfLevels = floor(srcPixel/amountPerLevel);
	vec4		outColor = numOfLevels * (vec4(1.0) / (vec4(levels) - vec4(1.0)));
	outColor.a = 1.0;
	gl_FragColor = outColor;
}
