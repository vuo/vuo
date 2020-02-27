/*{
	"DESCRIPTION": "buffers the last 3 frames and draws a 2x2 grid of the 4 current frames available",
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
			"NAME": "lag",
			"TYPE": "bool",
			"DEFAULT": 1.0
		}
	],
	"PERSISTENT_BUFFERS": {
		"buffer1": {
			"WIDTH": "$WIDTH/2.0",
			"HEIGHT": "$HEIGHT/2.0"
		},
		"buffer2": {
			"WIDTH": "$WIDTH/2.0",
			"HEIGHT": "$HEIGHT/2.0"
		},
		"buffer3": {
			"WIDTH": "$WIDTH/2.0",
			"HEIGHT": "$HEIGHT/2.0"
		}
	},
	"PASSES": [
		{
			"TARGET":"buffer3"
		},
		{
			"TARGET":"buffer2"
		},
		{
			"TARGET":"buffer1"
		},
		{
		
		}
	]
	
}*/

void main()
{
	//	first pass: read the "buffer2" into "buffer3"
	//	apply lag on each pass
	if (PASSINDEX == 0)	{
		if ((lag == false)||(mod(floor(TIME*60.0+2.0),6.0)<=1.0))	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer2);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer3);
		}
	}
	//	first pass: read the "buffer1" into "buffer2"
	else if (PASSINDEX == 1)	{
		if ((lag == false)||(mod(floor(TIME*60.0+2.0),6.0)<=1.0))	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer1);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer2);
		}
	}
	//	third pass: read from "inputImage" into "buffer1"
	else if (PASSINDEX == 2)	{
		if ((lag == false)||(mod(floor(TIME*60.0+2.0),6.0)<=1.0))	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(inputImage);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer1);
		}
	}
	else if (PASSINDEX == 3)	{
		//	Figure out which section I'm in and draw the appropriate buffer there
		vec2 tex = vv_FragNormCoord;
		vec4 color = vec4(0.0);
		//	TL – buffer1
		if ((tex.x < 0.5) && (tex.y > 0.5))	{
			tex.x = tex.x * 2.0;
			tex.y = (tex.y - 0.5) * 2.0;
			color = IMG_NORM_PIXEL(inputImage, tex);
		}
		//	TR – buffer2
		else if ((tex.x > 0.5) && (tex.y > 0.5))	{
			tex.x = (tex.x - 0.5) * 2.0;
			tex.y = (tex.y - 0.5) * 2.0;
			
			color = IMG_NORM_PIXEL(buffer1, tex);		
		}
		//	BR – buffer2
		else if ((tex.x < 0.5) && (tex.y < 0.5))	{
			tex = tex * 2.0;
			
			color = IMG_NORM_PIXEL(buffer2, tex);		
		}
		else	{
			tex.x = (tex.x - 0.5) * 2.0;
			tex.y = tex.y * 2.0;
			
			color = IMG_NORM_PIXEL(buffer3, tex);		
		}
		//	BL - buffer3
		gl_FragColor = color;
	}
}
