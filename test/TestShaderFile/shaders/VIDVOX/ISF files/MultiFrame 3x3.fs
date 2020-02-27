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
			"WIDTH": "$WIDTH/3.0",
			"HEIGHT": "$HEIGHT/3.0"
		},
		"buffer2": {
			"WIDTH": "$WIDTH/3.0",
			"HEIGHT": "$HEIGHT/3.0"
		},
		"buffer3": {
			"WIDTH": "$WIDTH/3.0",
			"HEIGHT": "$HEIGHT/3.0"
		},
		"buffer4": {
			"WIDTH": "$WIDTH/3.0",
			"HEIGHT": "$HEIGHT/3.0"
		},
		"buffer5": {
			"WIDTH": "$WIDTH/3.0",
			"HEIGHT": "$HEIGHT/3.0"
		},
		"buffer6": {
			"WIDTH": "$WIDTH/3.0",
			"HEIGHT": "$HEIGHT/3.0"
		},
		"buffer7": {
			"WIDTH": "$WIDTH/3.0",
			"HEIGHT": "$HEIGHT/3.0"
		},
		"buffer8": {
			"WIDTH": "$WIDTH/3.0",
			"HEIGHT": "$HEIGHT/3.0"
		}
	},
	"PASSES": [
		{
			"TARGET":"buffer8"
		},
		{
			"TARGET":"buffer7"
		},
		{
			"TARGET":"buffer6"
		},
		{
			"TARGET":"buffer5"
		},
		{
			"TARGET":"buffer4"
		},
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
	//	first pass: read the "buffer7" into "buffer8"
	//	apply lag on each pass
	if (PASSINDEX == 0)	{
		if ((lag == false)||(mod(floor(TIME*60.0+5.0),6.0)<=1.0))	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer7);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer8);
		}
	}
	else if (PASSINDEX == 1)	{
		if ((lag == false)||(mod(floor(TIME*60.0+3.0),6.0)<=1.0))	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer6);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer7);
		}
	}
	else if (PASSINDEX == 2)	{
		if ((lag == false)||(mod(floor(TIME*60.0+2.0),6.0)<=1.0))	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer5);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer6);
		}
	}
	else if (PASSINDEX == 3)	{
		if ((lag == false)||(mod(floor(TIME*60.0+5.0),6.0)<=1.0))	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer4);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer5);
		}
	}
	else if (PASSINDEX == 4)	{
		if ((lag == false)||(mod(floor(TIME*60.0+1.0),6.0)<=1.0))	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer3);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer4);
		}
	}
	else if (PASSINDEX == 5)	{
		if ((lag == false)||(mod(floor(TIME*60.0+3.0),6.0)<=1.0))	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer2);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer3);
		}
	}
	else if (PASSINDEX == 6)	{
		if ((lag == false)||(mod(floor(TIME*60.0+5.0),6.0)<=1.0))	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer1);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer2);
		}
	}
	else if (PASSINDEX == 7)	{
		if ((lag == false)||(mod(floor(TIME*60.0+3.0),6.0)<=1.0))	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(inputImage);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer1);
		}
	}
	else if (PASSINDEX == 8)	{
		//	Figure out which section I'm in and draw the appropriate buffer there
		vec2 tex = vv_FragNormCoord;
		vec4 color = vec4(0.0);
		//	TL – input
		if (tex.y > 0.667)	{
			if (tex.x < 0.333)	{
				tex.x = tex.x * 3.0;
				tex.y = (tex.y - 0.667) * 3.0;
				color = IMG_NORM_PIXEL(inputImage, tex);
			}
			//	TM – buffer1
			else if ((tex.x > 0.333) && (tex.x < 0.667))	{
				tex.x = (tex.x - 0.333) * 3.0;
				tex.y = (tex.y - 0.667) * 3.0;
			
				color = IMG_NORM_PIXEL(buffer1, tex);		
			}
			//	TL – buffer2
			else	{
				tex.x = (tex.x - 0.667) * 3.0;
				tex.y = (tex.y - 0.667) * 3.0;
			
				color = IMG_NORM_PIXEL(buffer2, tex);		
			}
		}
		//	BL - buffer3
		else if (tex.y > 0.333)	{
			if (tex.x < 0.333)	{
				tex.x = tex.x * 3.0;
				tex.y = (tex.y - 0.333) * 3.0;
				color = IMG_NORM_PIXEL(buffer3, tex);
			}
			//	TM – buffer1
			else if ((tex.x > 0.333) && (tex.x < 0.667))	{
				tex.x = (tex.x - 0.333) * 3.0;
				tex.y = (tex.y - 0.333) * 3.0;
			
				color = IMG_NORM_PIXEL(buffer4, tex);		
			}
			//	TL – buffer2
			else	{
				tex.x = (tex.x - 0.667) * 3.0;
				tex.y = (tex.y - 0.333) * 3.0;
			
				color = IMG_NORM_PIXEL(buffer5, tex);		
			}	
		}
		else	{
			if (tex.x < 0.333)	{
				tex.x = tex.x * 3.0;
				tex.y = tex.y * 3.0;
				color = IMG_NORM_PIXEL(buffer6, tex);
			}
			//	TM – buffer1
			else if ((tex.x > 0.333) && (tex.x < 0.667))	{
				tex.x = (tex.x - 0.333) * 3.0;
				tex.y = tex.y * 3.0;
			
				color = IMG_NORM_PIXEL(buffer7, tex);		
			}
			//	TL – buffer2
			else	{
				tex.x = (tex.x - 0.667) * 3.0;
				tex.y = tex.y * 3.0;
			
				color = IMG_NORM_PIXEL(buffer8, tex);		
			}			
		}
		
		gl_FragColor = color;
	}
}
