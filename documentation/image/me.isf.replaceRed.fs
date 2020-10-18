/*{
	"ISFVSN":"2.0",
	"TYPE":"IMAGE",
	"INPUTS":[
		{
			"NAME":"inputImage",
			"TYPE":"image",
			"LABEL":"Image"
		},
		{
			"NAME":"Red",
			"TYPE":"float"
		}
	]
}*/

void main()
{
	vec4 c = IMG_NORM_PIXEL(inputImage, isf_FragNormCoord.xy);
	gl_FragColor = vec4(Red,c.g,c.b,c.a);
}
