/*{
	"CREDIT": "by VIDVOX",
	"CATEGORIES": [
		"Glitch"
	],
	"INPUTS": [
		{
			"NAME": "inputImage",
			"TYPE": "image"
		},
		{
			"NAME": "glitch_size",
			"LABEL": "Size",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 0.5,
			"DEFAULT": 0.1
		},
		{
			"NAME": "glitch_horizontal",
			"LABEL": "Horizontal Amount",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 0.2
		},
		{
			"NAME": "glitch_vertical",
			"LABEL": "Vertical Amount",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 1.0,
			"DEFAULT": 0.0
		},
		{
			"NAME": "randomize_size",
			"LABEL": "Randomize Size",
			"TYPE": "bool",
			"DEFAULT": 1.0
		},
		{
			"NAME": "randomize_zoom",
			"LABEL": "Randomize Zoom",
			"TYPE": "bool",
			"DEFAULT": 0.0
		},
		{
			"NAME": "offset",
			"LABEL": "Offset",
			"TYPE": "point2D",
			"DEFAULT": [
				0,
				0
			]
		}
	]
}*/

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
	vec2 xy; 
	xy.x = vv_FragNormCoord[0];
	xy.y = vv_FragNormCoord[1];
	
	//	quantize the xy to the glitch_amount size
	//xy = floor(xy / glitch_size) * glitch_size;
	vec2 random;

	float local_glitch_size = glitch_size;
	float random_offset = 0.0;
	
	if (randomize_size)	{
		random_offset = mod(rand(vec2(TIME,TIME)), 1.0);
		local_glitch_size = random_offset * glitch_size;
	}
	
	if (local_glitch_size > 0.0)	{
		random.x = rand(vec2(floor(random_offset + xy.y / local_glitch_size) * local_glitch_size, TIME));
		random.y = rand(vec2(floor(random_offset + xy.x / local_glitch_size) * local_glitch_size, TIME));
	}
	else	{
		random.x = rand(vec2(xy.x, TIME));
		random.y = rand(vec2(xy.y, TIME));
	}
	
	if (randomize_zoom)	{
		if ((random.x < glitch_horizontal)&&(random.y < glitch_vertical))	{
			float level = rand(vec2(random.x, random.y)) / 5.0 + 0.90;
			xy = (xy - vec2(0.5))*(1.0/level) + vec2(0.5);
		}
		else if (random.x < glitch_horizontal)	{
			float level = (random.x) + 0.98;
			xy = (xy - vec2(0.5))*(1.0/level) + vec2(0.5);
		}
		else if (random.y < glitch_vertical)	{
			float level = (random.y) + 0.98;
			xy = (xy - vec2(0.5))*(1.0/level) + vec2(0.5);
		}
	}
	
	//	if doing a horizontal glitch do a random shift
	if ((random.x < glitch_horizontal)&&(random.y < glitch_vertical))	{
		vec2 shift = (offset / RENDERSIZE - 0.5);
		shift = shift * rand(shift + random);
		xy.x = mod(xy.x + random.x, 1.0);
		xy.y = mod(xy.y + random.y, 1.0);
		xy = xy + shift;
	}
	else if (random.x < glitch_horizontal)	{
		vec2 shift = (offset / RENDERSIZE - 0.5);
		shift = shift * rand(shift + random);
		xy = mod(xy + vec2(0.0, random.x) + shift, 1.0);
	}
	else if (random.y < glitch_vertical)	{
		vec2 shift = (offset / RENDERSIZE - 0.5);
		shift = shift * rand(shift + random);
		xy = mod(xy + vec2(random.y, 0.0) + shift, 1.0);
	}
	
	gl_FragColor = IMG_NORM_PIXEL(inputImage, xy);
}
