/*{
	"CREDIT": "by VIDVOX",
	"CATEGORIES": [
		"Color Effect"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		},
		{
			"NAME": "r",
			"TYPE": "bool",
			"DEFAULT": 1.0
		},
		{
			"NAME": "g",
			"TYPE": "bool",
			"DEFAULT": 1.0
		},
		{
			"NAME": "b",
			"TYPE": "bool",
			"DEFAULT": 1.0
		},
		{
			"NAME": "a",
			"TYPE": "bool",
			"DEFAULT": 0.0
		}
	]
}*/

void main() {
	vec4		srcPixel = IMG_THIS_PIXEL(inputImage);
	if (r)
		srcPixel.r = 1.0-srcPixel.r;
	if (g)
		srcPixel.g = 1.0-srcPixel.g;
	if (b)
		srcPixel.b = 1.0-srcPixel.b;
	if (a)
		srcPixel.a = 1.0-srcPixel.a;
	gl_FragColor = srcPixel;
}