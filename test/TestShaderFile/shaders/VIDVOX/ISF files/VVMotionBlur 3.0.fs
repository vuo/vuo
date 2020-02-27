/*{
	"DESCRIPTION": "this is basically identical to the demonstration of a persistent buffer",
	"CREDIT": "by zoidberg",
	"CATEGORIES": [
		"Blur"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		},
		{
			"NAME": "blurAmount",
			"TYPE": "float",
			"DEFAULT": 0.0
		}
	],
	"PERSISTENT_BUFFERS": [
		"bufferVariableNameA"
	],
	"PASSES": [
		{
			"TARGET":"bufferVariableNameA",
			"FLOAT": true
		},
		{
		
		}
	]
	
}*/

void main()
{
	vec4		freshPixel = IMG_PIXEL(inputImage,gl_FragCoord.xy);
	vec4		stalePixel = IMG_PIXEL(bufferVariableNameA,gl_FragCoord.xy);
	gl_FragColor = mix(freshPixel,stalePixel,blurAmount);
}
