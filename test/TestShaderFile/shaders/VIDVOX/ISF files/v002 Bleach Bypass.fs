/*{
	"CREDIT": "by v002",
	"CATEGORIES": [
		"Film", "v002"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		},
		{
			"NAME": "amount",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 0.0
		}
	]
}*/

//	Based on v002 bleach bypass – https://github.com/v002/v002-Film-Effects/

//constant variables.
const vec4 one = vec4(1.0);	
const vec4 two = vec4(2.0);
const vec4 lumcoeff = vec4(0.2125,0.7154,0.0721,0.0);


vec4 overlay(vec4 myInput, vec4 previousmix, vec4 amount)
{
	float luminance = dot(previousmix,lumcoeff);
	float mixamount = clamp((luminance - 0.45) * 10., 0., 1.);

	vec4 branch1 = two * previousmix * myInput;
	vec4 branch2 = one - (two * (one - previousmix) * (one - myInput));

	vec4 result = mix(branch1, branch2, vec4(mixamount) );

	return mix(previousmix, result, amount);
}

void main (void) 
{ 		
	vec4 input0 = IMG_THIS_PIXEL(inputImage);

	vec4 luma = vec4(dot(input0,lumcoeff));

	gl_FragColor = overlay(luma, input0, vec4(amount));

} 