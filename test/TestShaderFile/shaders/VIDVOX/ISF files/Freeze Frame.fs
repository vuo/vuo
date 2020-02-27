/*{
	"DESCRIPTION": "Holds the output on the captured freeze frame",
	"CREDIT": "by VIDVOX",
	"CATEGORIES": [
		"Utility"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		},
		{
			"NAME": "freeze",
			"TYPE": "bool",
			"DEFAULT": 0.0
		}
	],
	"PERSISTENT_BUFFERS": [
		"freezeBuffer"
	],
	"PASSES": [
		{
			"TARGET":"freezeBuffer"
		}
	]
	
}*/

//	Essentially the same as a feedback buffer where you can only set it to a mix of 1.0

void main()
{
	if (freeze)	{
		gl_FragColor = IMG_THIS_PIXEL(freezeBuffer);
	}
	else	{
		gl_FragColor = IMG_THIS_PIXEL(inputImage);
	}
}
