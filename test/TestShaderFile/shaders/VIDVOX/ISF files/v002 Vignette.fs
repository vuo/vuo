/*{
	"CREDIT": "by v002",
	"CATEGORIES": [
		"v002", "Film"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		},
		{
			"NAME": "vignette",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 0.5
		},
		{
			"NAME": "vignetteEdge",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 0.0
		},
		{
			"NAME": "vignetteMix",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 0.0
		}
	]
}*/


//	Based on v002 vignette – https://github.com/v002/v002-Film-Effects/



// create a black and white oval about the center of our image for our vignette
vec4 vignetteFucntion(vec2 normalizedTexcoord, float vignetteedge, float vignetteMix)
{
	normalizedTexcoord = 2.0 * normalizedTexcoord - 1.0; // - 1.0 to 1.0
	float r = length(normalizedTexcoord);
	vec4 vignette = (vec4(smoothstep(0.0, 1.0, pow(clamp(r - vignetteMix, 0.0, 1.0), 1.0 + vignetteedge * 10.0))));
	return clamp(1.0 - vignette, 0.0, 1.0);
}



void main (void) 
{ 		
	vec2 normcoord = vec2(vv_FragNormCoord[0],vv_FragNormCoord[1]);


	// make a vignette around our borders.
	vec4 vignetteResult = vignetteFucntion(normcoord, vignetteEdge, vignetteMix);

	// sharpen via unsharp mask (subtract image from blured image)
	vec4 input0 = IMG_THIS_PIXEL(inputImage);

	gl_FragColor = mix(input0,vignetteResult * input0, vignette);		
} 