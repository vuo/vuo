/*{
	"CREDIT": "by Carter Rosenberg",
	"CATEGORIES": [
		"Generator"
	],
	"INPUTS": [
		{
			"NAME": "offset",
			"TYPE": "float",
			"DEFAULT": 0.5
		},
		{
			"NAME": "vertical",
			"TYPE": "bool",
			"DEFAULT": 0.0
		},
		{
			"NAME": "startColor",
			"TYPE": "color",
			"DEFAULT": [
				1.0,
				0.75,
				0.0,
				1.0
			]
		},
		{
			"NAME": "endColor",
			"TYPE": "color",
			"DEFAULT": [
				0.0,
				0.25,
				0.75,
				1.0
			]
		}
	]
}*/




void main() {
	float mixOffset = offset * 2.0 - 1.0;
	if (vertical)	{
		mixOffset = mixOffset + vv_FragNormCoord[1];
	}
	else	{
		mixOffset = mixOffset + vv_FragNormCoord[0];
	}
	
	if (mixOffset > 1.0)	{
		mixOffset = 1.0 - (mixOffset - floor(mixOffset));
	}
	else if (mixOffset < 0.0)	{
		mixOffset = -1.0 * mixOffset;
	}
	
	gl_FragColor = mix(startColor,endColor,mixOffset);
}