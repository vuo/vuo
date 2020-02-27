/*{
	"CREDIT": "by zoidberg",
	"CATEGORIES": [
		"Color Effect"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		}
	]
}*/

void main() {
	vec4		srcPixel = IMG_THIS_PIXEL(inputImage);
	gl_FragColor = vec4(1.0-srcPixel.r, 1.0-srcPixel.g, 1.0-srcPixel.b, srcPixel.a);
}