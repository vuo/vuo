/*{
	"ISFVSN": "2.0",
	"LABEL": "Translate Image",
	"VSN": "1.0.0",
	"LICENSE": "Copyright © 2012–2022 Kosada Incorporated.  This code may be modified and distributed under the terms of the MIT License.  For more information, see https://vuo.org/license.",
	"KEYWORDS": [
		"filter",
		"move", "place", "displace", "shift", "reposition", "offset",
		"tile", "wrap",
		"crop", "trim", "snip", "clip", "cut",
		"edge", "outside", "border", "transparent", "black",
	],
	"EXAMPLES": [ ],
	"TYPE":"IMAGE",
	"INPUTS":[
		{"NAME":"image",              "TYPE":"image"},
		{"NAME":"translation",        "TYPE":"point2D", "DEFAULT":[0,0], "MIN":[-1,-1], "MAX":[1,1]},
		{"NAME":"horizontalExterior", "TYPE":"long", "DEFAULT":0, "VALUES":[0,1,2], "LABELS":["Transparent", "Black", "Wrap"]},
		{"NAME":"verticalExterior",   "TYPE":"long", "DEFAULT":0, "VALUES":[0,1,2], "LABELS":["Transparent", "Black", "Wrap"]},
	],
	"OUTPUTS":[{"NAME":"translatedImage"}],
}*/

void main()
{
	vec2 aspect = vec2(1., RENDERSIZE.y / RENDERSIZE.x);
	vec2 p = isf_FragNormCoord.xy - translation / aspect;

	// If we're in bounds, simply sample the texture.
	if (p.x >= 0. && p.x <= 1. && p.y >= 0. && p.y <= 1.)
		gl_FragColor = IMG_NORM_PIXEL(image, p);

	// If we're out of bounds on both axes…
	else if ((p.x < 0. && p.y < 0.)
		  || (p.x > 1. && p.y < 0.)
		  || (p.x < 0. && p.y > 1.)
		  || (p.x > 1. && p.y > 1.))
	{
		if (horizontalExterior == verticalExterior)
		{
			// If the exterior modes match,
			// also apply the exterior mode to the exterior corners.
			if (horizontalExterior == 0)
				gl_FragColor = vec4(0.);
			else if (horizontalExterior == 1)
				gl_FragColor = vec4(0., 0., 0., 1.);
			else
				gl_FragColor = IMG_NORM_PIXEL(image, mod(p, 1.));
		}
		else
			// If the exterior modes don't match,
			// make the exterior corners transparent,
			// since neither axis should take precedence.
			gl_FragColor = vec4(0.);
	}

	// If we're out of bounds on only the X axis…
	else if (p.x < 0. || p.x > 1.)
	{
		if (horizontalExterior == 0)
			gl_FragColor = vec4(0.);
		else if (horizontalExterior == 1)
			gl_FragColor = vec4(0., 0., 0., 1.);
		else
			gl_FragColor = IMG_NORM_PIXEL(image, mod(p, 1.));
	}

	// If we're out of bounds on only the Y axis…
	else
	{
		if (verticalExterior == 0)
			gl_FragColor = vec4(0.);
		else if (verticalExterior == 1)
			gl_FragColor = vec4(0., 0., 0., 1.);
		else
			gl_FragColor = IMG_NORM_PIXEL(image, mod(p, 1.));
	}
}
