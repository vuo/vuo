//	adapted from https://github.com/neilmendoza/ofxPostProcessing/blob/master/src/GodRaysPass.cpp


/*{
	"DESCRIPTION": "",
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
			"NAME": "lightDirDOTviewDir",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 0.5
		},
		{
			"NAME": "lightPositionOnScreen",
			"TYPE": "point2D",
			"DEFAULT": [
				0.5,
				0.5
			]
		},
		{
			"NAME": "quality",
			"VALUES": [
				5,
				10,
				25
			],
			"LABELS": [
				"Low",
				"Mid",
				"High"
			],
			"DEFAULT": 10,
			"TYPE": "long"
		}
	]
}*/


void main(void)
{
	vec4 origColor = IMG_THIS_PIXEL(inputImage);
	vec4 raysColor = IMG_THIS_PIXEL(inputImage);
	int NUM_SAMPLES = quality;

	if (lightDirDOTviewDir>0.0){
		float exposure	= 0.1/float(NUM_SAMPLES);
		float decay		= 1.0 ;
		float density	= 0.5;
		float weight	= 6.0;
		float illuminationDecay = 1.0;
		vec2		normSrcCoord;

		normSrcCoord.x = vv_FragNormCoord[0];
		normSrcCoord.y = vv_FragNormCoord[1];

		vec2 deltaTextCoord = vec2(normSrcCoord.st - lightPositionOnScreen/RENDERSIZE);
		vec2 textCoo = normSrcCoord;
		deltaTextCoord *= 1.0 / float(NUM_SAMPLES) * density;

		for(int i=0; i < NUM_SAMPLES ; i++)	{
			textCoo -= deltaTextCoord;
			vec4 tsample = IMG_NORM_PIXEL(inputImage, textCoo);
			tsample *= illuminationDecay * weight;
			raysColor += tsample;
			illuminationDecay *= decay;
		}
		raysColor *= exposure * lightDirDOTviewDir;
		float p = 0.3 *raysColor.g + 0.59*raysColor.r + 0.11*raysColor.b;
		gl_FragColor = origColor + p;
	}
	else {
		gl_FragColor = origColor;
	}
}