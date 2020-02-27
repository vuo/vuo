/*{
	"ISFVSN":"2.0",
	"TYPE":"IMAGE",
	"LABEL":"Lighter",
	"INPUTS":[
		{
			"NAME":"image1",
			"TYPE":"image"
		},
		{
			"NAME":"inputImage",
			"TYPE":"image"
		},
		{
			"NAME":"image2",
			"TYPE":"image"
		},
	],
}*/

void main()
{
	vec4 color1 = IMG_NORM_PIXEL(image1, isf_FragNormCoord);
	vec4 color2 = IMG_NORM_PIXEL(image2, isf_FragNormCoord);
	vec4 color3 = IMG_NORM_PIXEL(inputImage, isf_FragNormCoord);
	gl_FragColor = max(color1, color2);
	gl_FragColor = max(gl_FragColor, color3);
}
