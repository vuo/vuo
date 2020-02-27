/*{
	"ISFVSN":"1.0",
	"TYPE":"IMAGE",
	"LABEL":"ListLength",
	"INPUTS":[
		{
			"NAME":"points",
			"TYPE":"point2D[]",
			"DEFAULT":[
				[
					0.1,
					0.2
				],
				[
					0.3,
					0.4
				]
			]
		}
	]
}*/

void main()
{
	float d = 0.;
	for (int i = 0; i < LIST_LENGTH(points); ++i)
		d += 1. - pow(distance(isf_FragNormCoord, points[i]), .1);
	gl_FragColor = vec4(vec3(d), 1.);
}
