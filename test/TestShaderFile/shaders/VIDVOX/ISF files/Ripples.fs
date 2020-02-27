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
			"NAME": "level",
			"TYPE": "float",
			"MIN": 0.1,
			"MAX": 32.0,
			"DEFAULT": 1.0
		},
		{
			"NAME": "offset",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 0.0
		},
		{
			"NAME": "x_smear",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 0.0
		},
		{
			"NAME": "y_smear",
			"TYPE": "float",
			"MIN": 0.01,
			"MAX": 1.0,
			"DEFAULT": 0.0
		},
		{
			"NAME": "center",
			"TYPE": "point2D",
			"DEFAULT": [
				0.5,
				0.5
			]
		},
		{
			"NAME": "mode",
			"VALUES": [
				0,
				1
			],
			"LABELS": [
				"Single",
				"Double"
			],
			"DEFAULT": 0,
			"TYPE": "long"
		}
	]
}*/

const float pi = 3.14159265359;


float distance (vec2 center, vec2 pt)
{
	float tmp = pow(center.x-pt.x,2.0)+pow(center.y-pt.y,2.0);
	return pow(tmp,0.5);
}


void main() {
	vec2 uv = vec2(vv_FragNormCoord[0],vv_FragNormCoord[1]);
	vec2 texSize = RENDERSIZE;
	vec2 tc = uv * texSize;
	vec2 modifiedCenter = center;
	float r = distance(modifiedCenter, tc);
	float a = atan ((tc.y-modifiedCenter.y),(tc.x-modifiedCenter.x));
	float radius = 1.0;
	float radius_sized = radius * length(RENDERSIZE);
	
	tc -= modifiedCenter;

	if (r < radius_sized) 	{
		float percent = 1.0-(radius_sized - r) / radius_sized;
		float adjustedOffset = offset * 2.0 * pi;
		if (mode == 0)	{
			tc.x = r*(1.0+sin(percent * level * 2.0 * pi + adjustedOffset))/2.0 * cos(a);
			tc.y = r*(1.0+sin(percent * level * 2.0 * pi + adjustedOffset))/2.0 * sin(a);
		}
		else if (mode == 1)	{
			tc.x = r*(1.0+sin(percent * level * 2.0 * pi * cos(adjustedOffset + percent * percent * level * 2.0 * pi)))/2.0 * cos(a);
			tc.y = r*(1.0+sin(percent * level * 2.0 * pi * cos(adjustedOffset + percent * percent * level * 2.0 * pi)))/2.0 * sin(a);
		}
		tc.x = mix(uv.x, tc.x, max(1.0-x_smear,0.001));
		tc.y = mix(uv.y, tc.y, max(1.0-y_smear,0.001));
	}
	tc += modifiedCenter;
	vec2 loc = tc / texSize;

	if ((loc.x < 0.0)||(loc.y < 0.0)||(loc.x > 1.0)||(loc.y > 1.0))	{
		gl_FragColor = vec4(0.0);
	}
	else	{
		gl_FragColor = IMG_NORM_PIXEL(inputImage, loc);
	}
}
