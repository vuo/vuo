/*{
	"DESCRIPTION": "Visualizes an FFT analysis image with custom set colors for frequency domain",
	"CREDIT": "by VIDVOX",
	"CATEGORIES": [
		"Generator"
	],
	"INPUTS": [
		{
			"NAME": "fftImage",
			"TYPE": "image"
		},
		{
			"NAME": "waveImage",
			"TYPE": "image"
		},		
		{
			"NAME": "gainFFT",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 5.0,
			"DEFAULT": 1.0
		},
		{
			"NAME": "rangeFFT",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 1.0
		},
		{
			"NAME": "waveSize",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 0.5,
			"DEFAULT": 0.15
		},
		{
			"NAME": "vertical",
			"TYPE": "bool",
			"DEFAULT": 1.0
		},
		{
			"NAME": "color1",
			"TYPE": "color",
			"DEFAULT": [
				1.0,
				0.125,
				0.25,
				1.0
			]
		},
		{
			"NAME": "color2",
			"TYPE": "color",
			"DEFAULT": [
				0.0,
				0.5,
				1.0,
				1.0
			]
		},
		{
			"NAME": "color3",
			"TYPE": "color",
			"DEFAULT": [
				0.0,
				1.0,
				0.0,
				1.0
			]
		},
		{
			"NAME": "wavecolor",
			"TYPE": "color",
			"DEFAULT": [
				1.0,
				1.0,
				1.0,
				1.0
			]
		}
	]
}*/



void main() {
	
	vec2 loc = vv_FragNormCoord;

	if (vertical)	{
		loc.x = vv_FragNormCoord[1];
		loc.y = vv_FragNormCoord[0];
	}
	
	vec4 mixColor = color1;
	
	if (loc.y > 0.5)	{
		mixColor = mix (color2,color3,(loc.y-0.5)*2.0);
	}
	else	{
		mixColor = mix (color1,color2,(loc.y*2.0));
	}
	
	//	the fftImage is 256 steps
	loc.y = loc.y * rangeFFT;
	
	vec4 fft = IMG_NORM_PIXEL(fftImage, vec2(loc.y,0.5));
	fft = mixColor * fft;
	fft.rgb = gainFFT * fft.rgb;
	vec4 wave = IMG_NORM_PIXEL(waveImage, vec2(loc.y,0.5));
	fft += ((1.0 - smoothstep(0.0, waveSize, abs(wave - loc.x))) * wavecolor) * wavecolor.a;
	gl_FragColor = fft;
}