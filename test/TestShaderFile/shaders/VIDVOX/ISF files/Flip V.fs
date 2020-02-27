/*{
	"CREDIT": "by VIDVOX",
	"CATEGORIES": [
		"Geometry Adjustment"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		}
	]
}*/

void main() {
	vec2		normSrcCoord;

	normSrcCoord.x = vv_FragNormCoord[0];
	normSrcCoord.y = vv_FragNormCoord[1];

	normSrcCoord.y = (1.0-normSrcCoord.y);

	gl_FragColor = IMG_NORM_PIXEL(inputImage, normSrcCoord);
}