/*{
	"DESCRIPTION": "Buffers 8 recent frames",
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
			"NAME": "inputDelay",
			"LABEL": "Buffer",
			"TYPE": "color"
		},
		{
			"NAME": "inputRate",
			"LABEL": "Buffer Lag",
			"TYPE": "float",
			"MIN": 0.0,
			"MAX": 20.0,
			"DEFAULT": 1.0
		}
	],
	"PERSISTENT_BUFFERS": [
		"lastRow",
		"buffer1",
		"buffer2",
		"buffer3",
		"buffer4",
		"buffer5",
		"buffer6",
		"buffer7",
		"buffer8"
	],
	"PASSES": [
		{
			"TARGET":"lastRow",
			"WIDTH:": 1,
			"HEIGHT": 1,
			"DESCRIPTION": "this buffer stores the last frame's odd / even state"
		},
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
	//	if this is the first pass, i'm going to read the position from the "lastRow" image, and write a new position based on this and the hold variables
	if (PASSINDEX == 0)	{
		vec4		srcPixel = IMG_PIXEL(lastRow,vec2(0.5));
		//	i'm only using the X and Y components, which are the X and Y offset (normalized) for the frame
		if (inputRate == 0.0)	{
			srcPixel.x = 0.0;
			srcPixel.y = 0.0;
		}
		else if (inputRate <= 1.0)	{
			srcPixel.x = (srcPixel.x) > 0.5 ? 0.0 : 1.0;
			srcPixel.y = 0.0;
		}
		else {
			srcPixel.x = srcPixel.x + 1.0 / inputRate + srcPixel.y;
			if (srcPixel.x > 1.0)	{
				srcPixel.y = mod(srcPixel.x, 1.0);
				srcPixel.x = 0.0;
			}
		}
		gl_FragColor = srcPixel;
	}
	if (PASSINDEX == 1)	{
		vec4		lastRow = IMG_PIXEL(lastRow,vec2(0.5));
		if (lastRow.x == 0.0)	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer7);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer8);
		}
	}
	else if (PASSINDEX == 2)	{
		vec4		lastRow = IMG_PIXEL(lastRow,vec2(0.5));
		if (lastRow.x == 0.0)	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer6);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer7);
		}
	}
	else if (PASSINDEX == 3)	{
		vec4		lastRow = IMG_PIXEL(lastRow,vec2(0.5));
		if (lastRow.x == 0.0)	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer5);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer6);
		}
	}
	else if (PASSINDEX == 4)	{
		vec4		lastRow = IMG_PIXEL(lastRow,vec2(0.5));
		if (lastRow.x == 0.0)	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer4);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer5);
		}
	}
	else if (PASSINDEX == 5)	{
		vec4		lastRow = IMG_PIXEL(lastRow,vec2(0.5));
		if (lastRow.x == 0.0)	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer3);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer4);
		}
	}
	else if (PASSINDEX == 6)	{
		vec4		lastRow = IMG_PIXEL(lastRow,vec2(0.5));
		if (lastRow.x == 0.0)	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer2);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer3);
		}
	}
	else if (PASSINDEX == 7)	{
		vec4		lastRow = IMG_PIXEL(lastRow,vec2(0.5));
		if (lastRow.x == 0.0)	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer1);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer2);
		}
	}
	else if (PASSINDEX == 8)	{
		vec4		lastRow = IMG_PIXEL(lastRow,vec2(0.5));
		if (lastRow.x == 0.0)	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(inputImage);
		}
		else	{
			gl_FragColor = IMG_THIS_NORM_PIXEL(buffer1);
		}
	}
	else if (PASSINDEX == 9)	{
		//	Figure out which section I'm in and draw the appropriate buffer there
		vec2 tex = vv_FragNormCoord;
		vec4 color = vec4(0.0);
		vec4 pixelBuffer = inputDelay * 9.0;

		if (pixelBuffer.r < 1.0)	{
			color.r = IMG_NORM_PIXEL(inputImage, tex).r;
		}
		else if (pixelBuffer.r < 2.0)	{
			color.r = IMG_NORM_PIXEL(buffer1, tex).r;
		}
		else if (pixelBuffer.r < 3.0)	{
			color.r = IMG_NORM_PIXEL(buffer2, tex).r;
		}
		else if (pixelBuffer.r < 4.0)	{
			color.r = IMG_NORM_PIXEL(buffer3, tex).r;
		}
		else if (pixelBuffer.r < 5.0)	{
			color.r = IMG_NORM_PIXEL(buffer4, tex).r;
		}
		else if (pixelBuffer.r < 6.0)	{
			color.r = IMG_NORM_PIXEL(buffer5, tex).r;
		}
		else if (pixelBuffer.r < 7.0)	{
			color.r = IMG_NORM_PIXEL(buffer6, tex).r;
		}
		else if (pixelBuffer.r < 8.0)	{
			color.r = IMG_NORM_PIXEL(buffer7, tex).r;
		}
		else	{
			color.r = IMG_NORM_PIXEL(buffer8, tex).r;
		}
		
		if (pixelBuffer.g < 1.0)	{
			color.g = IMG_NORM_PIXEL(inputImage, tex).g;
		}
		else if (pixelBuffer.g < 2.0)	{
			color.g = IMG_NORM_PIXEL(buffer1, tex).g;
		}
		else if (pixelBuffer.g < 3.0)	{
			color.g = IMG_NORM_PIXEL(buffer2, tex).g;
		}
		else if (pixelBuffer.g < 4.0)	{
			color.g = IMG_NORM_PIXEL(buffer3, tex).g;
		}
		else if (pixelBuffer.g < 5.0)	{
			color.g = IMG_NORM_PIXEL(buffer4, tex).g;
		}
		else if (pixelBuffer.g < 6.0)	{
			color.g = IMG_NORM_PIXEL(buffer5, tex).g;
		}
		else if (pixelBuffer.g < 7.0)	{
			color.g = IMG_NORM_PIXEL(buffer6, tex).g;
		}
		else if (pixelBuffer.g < 8.0)	{
			color.g = IMG_NORM_PIXEL(buffer7, tex).g;
		}
		else	{
			color.g = IMG_NORM_PIXEL(buffer8, tex).g;
		}
		
		if (pixelBuffer.b < 1.0)	{
			color.b = IMG_NORM_PIXEL(inputImage, tex).b;
		}
		else if (pixelBuffer.b < 2.0)	{
			color.b = IMG_NORM_PIXEL(buffer1, tex).b;
		}
		else if (pixelBuffer.b < 3.0)	{
			color.b = IMG_NORM_PIXEL(buffer2, tex).b;
		}
		else if (pixelBuffer.b < 4.0)	{
			color.b = IMG_NORM_PIXEL(buffer3, tex).b;
		}
		else if (pixelBuffer.b < 5.0)	{
			color.b = IMG_NORM_PIXEL(buffer4, tex).b;
		}
		else if (pixelBuffer.b < 6.0)	{
			color.b = IMG_NORM_PIXEL(buffer5, tex).b;
		}
		else if (pixelBuffer.b < 7.0)	{
			color.b = IMG_NORM_PIXEL(buffer6, tex).b;
		}
		else if (pixelBuffer.b < 8.0)	{
			color.b = IMG_NORM_PIXEL(buffer7, tex).b;
		}
		else	{
			color.b = IMG_NORM_PIXEL(buffer8, tex).b;
		}
		
		if (pixelBuffer.a < 1.0)	{
			color.a = IMG_NORM_PIXEL(inputImage, tex).a;
		}
		else if (pixelBuffer.a < 2.0)	{
			color.a = IMG_NORM_PIXEL(buffer1, tex).a;
		}
		else if (pixelBuffer.a < 3.0)	{
			color.a = IMG_NORM_PIXEL(buffer2, tex).a;
		}
		else if (pixelBuffer.a < 4.0)	{
			color.a = IMG_NORM_PIXEL(buffer3, tex).a;
		}
		else if (pixelBuffer.a < 5.0)	{
			color.a = IMG_NORM_PIXEL(buffer4, tex).a;
		}
		else if (pixelBuffer.a < 6.0)	{
			color.a = IMG_NORM_PIXEL(buffer5, tex).a;
		}
		else if (pixelBuffer.a < 7.0)	{
			color.a = IMG_NORM_PIXEL(buffer6, tex).a;
		}
		else if (pixelBuffer.a < 8.0)	{
			color.a = IMG_NORM_PIXEL(buffer7, tex).a;
		}
		else	{
			color.a = IMG_NORM_PIXEL(buffer8, tex).a;
		}

		gl_FragColor = color;
	}
}
