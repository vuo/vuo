/*{
	"ISFVSN":"1.0",
	"TYPE":"IMAGE",
	"LABEL":"Ripple Image",
	"VSN":"2.1.0",
	"KEYWORDS":[
		"wave",
		"sinusoidal",
		"sine",
		"cosine",
		"undulate",
		"ruffle",
		"swish",
		"swing",
		"flap",
		"sway",
		"billow",
		"water",
		"filter"
	],
	"INPUTS":[
		{
			"NAME":"inputImage",
			"TYPE":"image",
			"LABEL":"Image"
		},
		{
			"NAME":"angle",
			"TYPE":"float",
			"DEFAULT":135.0,
			"MIN":0.0,
			"MAX":360.0,
			"STEP":1.0
		},
		{
			"NAME":"amplitude",
			"TYPE":"float",
			"DEFAULT":0.1,
			"MIN":0.0,
			"MAX":1.0,
			"STEP":0.05
		},
		{
			"NAME":"wavelength",
			"TYPE":"float",
			"DEFAULT":0.05,
			"MIN":0.000001,
			"MAX":0.05,
			"STEP":0.001
		},
		{
			"NAME":"phase",
			"TYPE":"float",
			"DEFAULT":0.0,
			"MIN":0.0,
			"MAX":1.0,
			"STEP":0.01
		}
	],
	"OUTPUTS":[
		{
			"NAME":"rippledImage",
			"LABEL":"Rippled Image"
		}
	]
}*/

varying float _wavelength;
varying float _angle;
varying float _phase;

void main()
{
	float samplerPhase = cos(_angle)*isf_FragNormCoord.x + sin(_angle)*isf_FragNormCoord.y;
	float offset = sin(samplerPhase/_wavelength + _phase) * amplitude;
	gl_FragColor = IMG_NORM_PIXEL(inputImage, isf_FragNormCoord + vec2(cos(_angle)*offset,sin(_angle)*offset));
}
