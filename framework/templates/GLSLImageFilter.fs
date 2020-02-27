/*{
	"ISFVSN":"2.0",
	"TYPE":"IMAGE",
	"INPUTS":[
		{
			"NAME":"inputImage",
			"TYPE":"image",
			"LABEL":"Image"
		}
	]
}*/

void main()
{
	gl_FragColor = IMG_NORM_PIXEL(inputImage, isf_FragNormCoord.xy);
}
