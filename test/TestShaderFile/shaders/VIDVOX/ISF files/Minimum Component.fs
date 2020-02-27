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
	float		minComponent = min(srcPixel.r, min(srcPixel.g, srcPixel.b));
	gl_FragColor = vec4(minComponent, minComponent, minComponent, srcPixel.a);
}