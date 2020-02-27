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
			"NAME": "intensity",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 1.0
		},
		{
			"NAME": "color",
			"TYPE": "color",
			"DEFAULT": [
				0.6,
				0.45,
				0.3,
				1.0
			]
		}
	]
}*/


const vec4		lumcoeff = vec4(0.299, 0.587, 0.114, 0.0);

void main() {
	vec4		srcPixel = IMG_THIS_PIXEL(inputImage);
	float		luminance = dot(srcPixel,lumcoeff);
	//float		luminance = (srcPixel.r + srcPixel.g + srcPixel.b)/3.0;
	vec4		dstPixel = (luminance >= 0.50)
		? mix(color, vec4(1,1,1,1), (luminance-0.5)*2.0)
		: mix(vec4(0,0,0,1), color, luminance*2.0);
	gl_FragColor = mix(srcPixel, dstPixel, intensity);
}