/*{
	"CREDIT": "by VIDVOX",
	"CATEGORIES": [
		"Color Effect"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		},
		{
			"NAME": "darkColor",
			"TYPE": "color",
			"DEFAULT": [
				0.00,
				0.164,
				1.0,
				1.0
			]
		},
		{
			"NAME": "midColor",
			"TYPE": "color",
			"DEFAULT": [
				0.00,
				1.00,
				0.00,
				1.0
			]
		},
		{
			"NAME": "brightColor",
			"TYPE": "color",
			"DEFAULT": [
				1.0,
				0.00,
				0.00,
				1.0
			]
		}
	]
}*/


//	partly adapted from http://coding-experiments.blogspot.com/2010/10/thermal-vision-pixel-shader.html


void main ()	{
	vec4 pixcol = IMG_THIS_PIXEL(inputImage);
	
	vec4 colors[4];
	colors[0] = vec4(0.0,0.0,0.0,1.0);
	colors[1] = darkColor;
	colors[2] = midColor;
	colors[3] = brightColor;
	float lum = (pixcol.r+pixcol.g+pixcol.b)/3.;
	int ix = 0; 

	if (lum > 0.66)	{
		ix = 2;
	}
	else if (lum > 0.33)	{
		ix = 1;
	}
	
	vec4 thermal = mix(colors[ix],colors[ix+1],(lum-float(ix)*0.33)/0.33);
	gl_FragColor = thermal;

}