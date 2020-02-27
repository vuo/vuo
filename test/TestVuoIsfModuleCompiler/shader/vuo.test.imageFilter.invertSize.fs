/*{
	"ISFVSN":"2.0",
	"TYPE":"IMAGE",
	"LABEL":"Image Filter with Input Ports",
	"INPUTS":[
		{
			"NAME":"inputImage",
			"TYPE":"image",
			"LABEL":"Image"
		},
		{
			"TYPE":"size"
		},
	],
}*/

void main()
{
	vec4 color = IMG_NORM_PIXEL(inputImage, isf_FragNormCoord);
	color.rgb /= color.a > 0. ? color.a : 1.;
	color.rgb = 1. - color.rgb;
	color.rgb *= color.a;
	gl_FragColor = color;
}
