/*{
	"CREDIT": "by carter rosenberg",
	"CATEGORIES": [
		"Distortion Effect"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		},
		{
			"NAME": "levelX",
			"TYPE": "float",
			"MIN": 0.01,
			"MAX": 10.0,
			"DEFAULT": 1.0
		},
		{
			"NAME": "levelY",
			"TYPE": "float",
			"MIN": 0.01,
			"MAX": 10.0,
			"DEFAULT": 1.0
		},
		{
			"NAME": "center",
			"TYPE": "point2D",
			"DEFAULT": [
				0.5,
				0.5
			]
		}
	]
}*/

void main() {
	vec2		loc;
	vec2		modifiedCenter;
	
	loc = vv_FragNormCoord;
	modifiedCenter = center / RENDERSIZE;
	loc.x = (loc.x - modifiedCenter.x)*(1.0/levelX) + modifiedCenter.x;
	loc.y = (loc.y - modifiedCenter.y)*(1.0/levelY) + modifiedCenter.y;
	if ((loc.x < 0.0)||(loc.y < 0.0)||(loc.x > 1.0)||(loc.y > 1.0))	{
		gl_FragColor = vec4(0.0);
	}
	else	{
		gl_FragColor = IMG_NORM_PIXEL(inputImage,loc);
	}
}
