/*{
	"ISFVSN": "2.0",
	"LABEL": "Streak Image",
	"VSN": "1.0.0",
	"LICENSE": "Copyright © 2012–2022 Kosada Incorporated.  This code may be modified and distributed under the terms of the MIT License.  For more information, see https://vuo.org/license.",
	"KEYWORDS": [
		"filter",
		"affine clamp", "pixel stretch",
		"edge", "outside", "border",
		"hold", "elastic", "rubber", "extend", "lengthen", "elongate", "expand", "spread",
		"glitch", "corruption", "error", "broken", "garbage", "artifacts",
		"crop", "trim", "snip", "clip", "cut",
	],
	"EXAMPLES": [ ],
	"TYPE":"IMAGE",
	"INPUTS":[
		{"NAME":"image",         "TYPE":"image"},
		{"NAME":"left",          "TYPE":"float", "DEFAULT":0.25, "MIN":  0, "MAX": 1, "STEP":0.01},
		{"NAME":"right",         "TYPE":"float", "DEFAULT":0.75, "MIN":  0, "MAX": 1, "STEP":0.01},
		{"NAME":"top",           "TYPE":"float", "DEFAULT":0.75, "MIN":  0, "MAX": 1, "STEP":0.01},
		{"NAME":"bottom",        "TYPE":"float", "DEFAULT":0.25, "MIN":  0, "MAX": 1, "STEP":0.01},
		{"NAME":"sharpness",     "TYPE":"float", "DEFAULT":1,    "MIN":  0, "MAX": 1, "STEP":0.1 },
		{"NAME":"angle",         "TYPE":"float", "DEFAULT":0,    "MIN":-45, "MAX":45, "STEP":5   },
		{"NAME":"fade",          "TYPE":"float", "DEFAULT":0,    "MIN":  0, "MAX": 1, "STEP":0.1 },
		{"NAME":"fadeSharpness", "TYPE":"float", "DEFAULT":0,    "MIN":  0, "MAX": 1, "STEP":0.1 },
	],
	"OUTPUTS":[{"NAME":"streakedImage"}],
}*/

uniform float aspectRatio;

// Based on "Digital Dynamic Range Compressor Design"
// by Giannoulis, Massberg, and Reiss,
// J. Audio Eng. Soc., Vol. 60, No. 6, 2012 June,
// section 2.2, equation 4.
float softknee(float t, float xg, float w)
{
	float r = .5;
	float piecewise = 2 * (xg - t);
	float yg = piecewise < -w ? xg
			 : piecewise >  w ? t + (xg - t) / r
		     : xg + (1 / r - 1) * pow(xg - t + w/2, 2) / (2 * w);

	return yg;
}

vec2 rotate(vec2 p, float angle)
{
	float x = p.x - .5;
	float y = (p.y - .5) / aspectRatio;
	p.x = x * cos(angle) - y * sin(angle);
	p.y = y * cos(angle) + x * sin(angle);
	p.y *= aspectRatio;
	p += .5;
	return p;
}

void main()
{
	vec2 tc = isf_FragNormCoord.xy;
	float fw = fwidth(tc.x);
	float angleRadians = angle * 3.14159/180.;

	tc = rotate(tc, -angleRadians);

	// Left/Right clamping.
	float right2 = max(left, right);
	float xwf = max(fw, min(right2 - left, 1 - fadeSharpness) / 2);
	float xf = smoothstep(left   - xwf, left   + xwf,  tc.x)
			 * smoothstep(right2 + xwf, right2 - xwf, tc.x);
	float xw = min(right2 - left, 1.0001 - sharpness);
	tc.x = tc.x < (left + right2) / 2
		? softknee(2 * left - tc.x, left, xw)
		: 2 * right2 - softknee(tc.x, right2, xw);

	// Top/Bottom clamping.
	float top2 = max(top, bottom);
	float ywf = max(fw, min(top2 - bottom, 1 - fadeSharpness) / 2);
	float yf = smoothstep(bottom - ywf, bottom + ywf,  tc.y)
			 * smoothstep(top2   + ywf, top2   - ywf, tc.y);
	float yw = min(top2 - bottom, 1.0001 - sharpness);
	tc.y = tc.y < (bottom + top2) / 2
		? softknee(2 * bottom - tc.y, bottom, yw)
		: 2 * top2 - softknee(tc.y, top2, yw);

	tc = rotate(tc, angleRadians);

	gl_FragColor = IMG_NORM_PIXEL(image, tc) * mix(1, xf * yf, fade);
}
