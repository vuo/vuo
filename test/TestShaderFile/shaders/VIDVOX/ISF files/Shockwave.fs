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
			"NAME": "positionVal",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 0.0
		},
		{
			"NAME": "distortion",
			"TYPE": "float",
			"MIN": 1.0,
			"MAX": 20.0,
			"DEFAULT": 10.0
		},
		{
			"NAME": "magnitude",
			"TYPE": "float",
			"MIN": 0.00,
			"MAX": 0.2,
			"DEFAULT": 0.08
		},
		{
			"NAME": "center",
			"TYPE": "point2D",
			"DEFAULT": [
				0.0,
				0.0
			]
		},
		{
			"NAME": "background",
			"TYPE": "bool",
			"DEFAULT": 1.0
		}
	]
	
}*/



void main()
{
	//	do this junk so that the ripple starts from nothing
	vec2 uv = vv_FragNormCoord.xy;
	vec2 texCoord = uv;
	vec2 mod_center = center / RENDERSIZE;
	vec4 color = vec4(0.0);
	float dist = distance(uv, mod_center);
	float adjustedTime = (positionVal * RENDERSIZE.x/RENDERSIZE.y - magnitude)/(1.0 - magnitude);

	if ( (dist <= (adjustedTime + magnitude)) && (dist >= (adjustedTime - magnitude)) ) 	{
		float diff = (dist - adjustedTime); 
		float powDiff = 1.0 - pow(abs(diff*distortion), 
								0.8); 
		float diffTime = diff  * powDiff; 
		vec2 diffUV = normalize(uv - mod_center); 
		texCoord = uv + (diffUV * diffTime);
		color = IMG_NORM_PIXEL(inputImage, texCoord);
	}
	else if (background)	{
		color = IMG_NORM_PIXEL(inputImage, texCoord);
	}
	gl_FragColor = color;
}
