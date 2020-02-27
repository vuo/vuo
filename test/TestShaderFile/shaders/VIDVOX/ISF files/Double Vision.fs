
/*{
	"CREDIT": "by VIDVOX",
	"CATEGORIES": [
		"Stylize"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		},
		{
			"NAME": "hShift",
			"LABEL": "H Shift",
			"TYPE": "float",
			"MIN": -0.05,
			"MAX": 0.05,
			"DEFAULT": 0.0
		},
		{
			"NAME": "vShift",
			"LABEL": "V Shift",
			"TYPE": "float",
			"MIN": -0.05,
			"MAX": 0.05,
			"DEFAULT": 0.0
		},
		{
			"NAME": "mixAmount1",
			"LABEL": "Shift Mix",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 0.5
		},
		{
			"NAME": "mixAmount2",
			"LABEL": "Original Mix",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 0.5
		}
	]
}*/



void main()
{
	vec2 loc = vv_FragNormCoord;
	vec2 shift = vec2(hShift, vShift);

	//	zoom slightly so that there aren't out of range pixels
	float zoomAmount = 1.0 + 2.0 * max(hShift, vShift);
	vec2 modifiedCenter = vec2(0.5,0.5);
	loc.x = (loc.x - modifiedCenter.x)*(1.0/zoomAmount) + modifiedCenter.x;
	loc.y = (loc.y - modifiedCenter.y)*(1.0/zoomAmount) + modifiedCenter.y;
	
	vec4 color = IMG_NORM_PIXEL(inputImage, vv_FragNormCoord);
	vec4 colorL = IMG_NORM_PIXEL(inputImage, clamp(loc - shift,0.0,1.0));
	vec4 colorR = IMG_NORM_PIXEL(inputImage, clamp(loc + shift,0.0,1.0));
	
	vec4 outColor = mix(min(colorL, colorR), max(colorL, colorR), mixAmount1);
	outColor =  mix(min(outColor, color), max(outColor, color), mixAmount2);
	
	gl_FragColor = outColor;
}