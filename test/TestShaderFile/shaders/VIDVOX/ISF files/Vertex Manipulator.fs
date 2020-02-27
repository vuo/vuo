/*{
	"DESCRIPTION": "Moves the vertex points to the specified locations without correction",
	"CREDIT": "VIDVOX",
	"CATEGORIES": [
		"Glitch"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		},
		{
			"NAME": "topleft",
			"LABEL": "Top Left",
			"TYPE": "point2D",
			"DEFAULT": [
				0.0,
				1.0
			]
		},
		{
			"NAME": "bottomleft",
			"LABEL": "Bottom Left",
			"TYPE": "point2D",
			"DEFAULT": [
				0.0,
				0.0
			]
		},
		{
			"NAME": "topright",
			"LABEL": "Top Right",
			"TYPE": "point2D",
			"DEFAULT": [
				1.0,
				1.0
			]
		},

		{
			"NAME": "bottomright",
			"LABEL": "Bottom Right",
			"TYPE": "point2D",
			"DEFAULT": [
				1.0,
				0.0
			]
		}
	]
}*/


void main()
{
	vec4		test = IMG_THIS_PIXEL(inputImage);
	gl_FragColor = test;
}
