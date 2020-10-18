/*{
	"ISFVSN": "2.0",
	"LABEL": "Streak Image Radially",
	"VSN": "1.0.0",
	"LICENSE": "Copyright © 2012–2020 Kosada Incorporated.  This code may be modified and distributed under the terms of the MIT License.  For more information, see https://vuo.org/license.",
	"KEYWORDS": [
		"filter",
		"circle splash distortion", "light tunnel distortion", "pixel stretch",
		"clamp", "edge", "outside", "border",
		"hold", "elastic", "rubber", "extend", "lengthen", "elongate", "expand", "spread",
		"glitch", "corruption", "error", "broken", "garbage", "artifacts",
		"crop", "trim", "snip", "clip", "cut",
		"circular", "oval", "polar",
	],
	"EXAMPLES": [ ],
	"TYPE":"IMAGE",
	"INPUTS":[
		{"NAME":"image",         "TYPE":"image"},
		{"NAME":"center",        "TYPE":"point2D", "DEFAULT":[0,0], "MIN":[-1,-1], "MAX":[1,1], "STEP":[0.01,0.01]},
		{"NAME":"radius",        "TYPE":"float",   "DEFAULT":0.25,  "MIN":0,       "MAX":1,     "STEP":0.01       },
		{"NAME":"twirlAngle",    "TYPE":"float",   "DEFAULT":0,     "MIN":-180,    "MAX":180,   "STEP":15         },
		{"NAME":"fade",          "TYPE":"float",   "DEFAULT":0,     "MIN":0,       "MAX":1,     "STEP":0.1        },
		{"NAME":"fadeSharpness", "TYPE":"float",   "DEFAULT":0,     "MIN":0,       "MAX":1,     "STEP":0.1        },
	],
	"OUTPUTS":[{"NAME":"streakedImage"}],
}*/

uniform float aspectRatio;

void main()
{
	vec2 tc = isf_FragNormCoord.xy;
	float fw = fwidth(tc.x) / 2;

	vec2 center2 = center * vec2(1., aspectRatio) / 2. + vec2(.5, .5);
	float radius2 = radius / 2.;
	vec2 tcc = (tc - vec2(.5, .5)) + vec2(.5, .5);

	tcc.y /= aspectRatio;
	center2.y /= aspectRatio;
	float r = distance(tcc, center2);
	float theta = atan(tcc.y - center2.y, tcc.x - center2.x);

	tc = min(r, radius2) * vec2(cos(theta), sin(theta));

	if (r > radius2)
	{
		float angle = twirlAngle * 3.14159 / 180.;
		float theta2 = (radius2 - r) * angle;
		float st = sin(theta2);
		float ct = cos(theta2);
		tc = vec2(tc.x * ct - tc.y * st, tc.x * st + tc.y * ct);
	}

	tc += center2;
	tc.y *= aspectRatio;

	float wf = max(fw, (1 - fadeSharpness) / 2);
	float f = mix(1, smoothstep(r - wf, r + wf, radius2), fade);
	gl_FragColor = IMG_NORM_PIXEL(image, tc) * f;
}
