/*{
	"DESCRIPTION": "",
	"CREDIT": "by VIDVOX",
	"CATEGORIES": [
		"Color Effect"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		}
	],
	"PERSISTENT_BUFFERS": [
		"lastState"
	],
	"PASSES": [
		{
			"TARGET":"lastState",
			"WIDTH:": 1,
			"HEIGHT": 1,
			"DESCRIPTION": "this buffer stores the last frame's time offset in the first component of its only pixel- note that it's requesting a FLOAT target buffer..."
		},
		{
			
		}
	]
	
}*/



void main()
{
	//	if this is the first pass, i'm going to read the position from the "lastPosition" image, and write a new position based on this and the hold variables
	if (PASSINDEX == 0)	{
		vec4		srcPixel = IMG_PIXEL(lastState,vec2(0.5));
		//	i'm only using the X, which is the last render time we reset
		srcPixel.r = (srcPixel.r == 0.0) ? 1.0 : 0.0;
		gl_FragColor = srcPixel;
	}
	//	else this isn't the first pass- read the position value from the buffer which stores it
	else	{
		vec4 lastStateVector = IMG_PIXEL(lastState,vec2(0.5));
		vec4 srcPixel = IMG_THIS_PIXEL(inputImage);
		srcPixel = (lastStateVector.r == 0.0) ? srcPixel : vec4(1.0-srcPixel.r, 1.0-srcPixel.g, 1.0-srcPixel.b, srcPixel.a);
		gl_FragColor = srcPixel;
	}
}
