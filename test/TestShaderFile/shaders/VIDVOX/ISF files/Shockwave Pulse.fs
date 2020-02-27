/*{
	"DESCRIPTION": "",
	"CREDIT": "by VIDVOX",
	"CATEGORIES": [
		"Distortion Effect"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		},
		{
			"NAME": "pulse",
			"TYPE": "event"
		},
		{
			"NAME": "rate",
			"LABEL": "rate",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 4.0,
			"DEFAULT": 1.0
		},
		{
			"NAME": "magnitude",
			"LABEL": "magnitude",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 0.2,
			"DEFAULT": 0.08
		},
		{
			"NAME": "distortion",
			"LABEL": "distortion",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 20.0,
			"DEFAULT": 10.0
		},
		{
			"NAME": "center",
			"TYPE": "point2D",
			"DEFAULT": [
				0.0,
				0.0
			]
		}
	],
	"PERSISTENT_BUFFERS": [
		"lastTime"
	],
	"PASSES": [
		{
			"TARGET":"lastTime",
			"WIDTH:": 1,
			"HEIGHT": 1,
			"FLOAT": true,
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
		vec4		srcPixel = IMG_PIXEL(lastTime,vec2(0.5));
		//	i'm only using the X, which is the last render time we reset
		srcPixel.r = (pulse) ? 0.0 : clamp(srcPixel.r + rate * 0.01,0.0,1.0);
		gl_FragColor = srcPixel;
	}
	//	else this isn't the first pass- read the position value from the buffer which stores it
	else	{
		vec2 uv = vv_FragNormCoord.xy;
		vec2 texCoord = uv;
		vec2 mod_center = center / RENDERSIZE;
		float distance = distance(uv, mod_center);
		vec4 lastPosVector = IMG_PIXEL(lastTime,vec2(0.5));
		float adustedTime = lastPosVector.r * (1.0+length(distance));

		if ( (distance <= (adustedTime + magnitude)) && (distance >= (adustedTime - magnitude)) ) 	{
			float diff = (distance - adustedTime); 
			float powDiff = 1.0 - pow(abs(diff*distortion), 
									0.8); 
			float diffTime = diff  * powDiff; 
			vec2 diffUV = normalize(uv - mod_center); 
			texCoord = uv + (diffUV * diffTime);
		} 
		gl_FragColor = IMG_NORM_PIXEL(inputImage, texCoord);
	}
}
