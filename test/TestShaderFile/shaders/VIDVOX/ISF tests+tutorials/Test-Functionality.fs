/*{
	"DESCRIPTION": "just colors the passed image opaque red, tests basic rendering functionality",
	"CREDIT": "by zoidberg",
	"CATEGORIES": [
		"TEST-GLSL FX"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		}
	]
}*/

void main()
{
	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}