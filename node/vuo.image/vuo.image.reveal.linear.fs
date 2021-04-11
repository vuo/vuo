/*{
	"ISFVSN": "2.0",
	"LABEL": "Reveal Image (Linear)",
	"VSN": "1.0.0",
	"LICENSE": "Copyright © 2012–2021 Kosada Incorporated.  This code may be modified and distributed under the terms of the MIT License.  For more information, see https://vuo.org/license.",
	"KEYWORDS": [
		"filter",
		"combine", "mix", "fade", "crossfade", "merge", "composite",
		"wipe", "swipe", "sweep", "swoosh", "ramp",
		"horizontal", "vertical", "diagonal",
		"motion", "transition", "mask",
		"crop", "trim", "snip", "clip", "cut",
	],
	"EXAMPLES": [ ],
	"TYPE":"IMAGE",
	"INPUTS":[
		{"NAME":"startImage", "TYPE":"image"},
		{"NAME":"endImage",   "TYPE":"image"},
		{"NAME":"progress",   "TYPE":"float", "DEFAULT":0.5, "MIN":0,    "MAX":1,   "STEP":0.1},
		{"NAME":"angle",      "TYPE":"float", "DEFAULT":45,  "MIN":-180, "MAX":180, "STEP":15 },
		{"NAME":"sharpness",  "TYPE":"float", "DEFAULT":0.8, "MIN":0,    "MAX":1,   "STEP":0.1},
	],
	"OUTPUTS":[{"NAME":"combinedImage"}],
}*/

void main()
{
	float angleRad = (angle - 90.) * 3.14159 / 180.;
	mat2 rotate = mat2(cos(angleRad), -sin(angleRad),
					   sin(angleRad),  cos(angleRad));

	vec2 aspect = vec2(1., RENDERSIZE.y / RENDERSIZE.x);
	float rotatedPosition = (((isf_FragNormCoord.xy - vec2(.5, .5)) * aspect) * rotate).y;

	float spread = 1. - sharpness;
	spread = max(fwidth(rotatedPosition), spread);
	spread /= 2.;

	float range = 1. + abs(aspect.y - 1.) * 2.;

	float pos = range / 2. - progress * (range + spread * 2.) + spread;

	float t = smoothstep(pos - spread, pos + spread, rotatedPosition);

	gl_FragColor = mix(
		IMG_SIZE(startImage).x > 0 ? IMG_NORM_PIXEL(startImage, isf_FragNormCoord.xy) : vec4(0.),
		IMG_SIZE(endImage).x   > 0 ? IMG_NORM_PIXEL(endImage,   isf_FragNormCoord.xy) : vec4(0.),
		t);
}
