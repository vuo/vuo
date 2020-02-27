/*{
	"ISFVSN":"2.0",
	"TYPE":"IMAGE",
	"INPUTS":[
		{
			"NAME":"startImage",
			"TYPE":"image",
			"LABEL":"Start Image"
		},
		{
			"NAME":"endImage",
			"TYPE":"image",
			"LABEL":"End Image"
		},
		{
			"NAME":"progress",
			"TYPE":"float",
			"LABEL":"Progress",
			"MIN":0,
			"MAX":1
		}
	]
}*/

void main()
{
	vec4 start = IMG_NORM_PIXEL(startImage, isf_FragNormCoord.xy);
	vec4 end   = IMG_NORM_PIXEL(endImage,   isf_FragNormCoord.xy);
	gl_FragColor = mix(start, end, progress);
}
