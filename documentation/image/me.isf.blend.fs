/*{
	"LABEL":"Blend Image Components",
   "INPUTS":[
      {
         "NAME":"image1",
         "TYPE":"image"
      },
      {
         "NAME":"image2",
         "TYPE":"image"
      },
      {
         "NAME":"blendType",
         "TYPE":"long",
         "VALUES":[0, 1],
         "LABELS":["Darker Component", "Lighter Component"]
      }
   ]
}*/

void main()
{
   vec4 color1 = IMG_THIS_NORM_PIXEL(image1);
   vec4 color2 = IMG_THIS_NORM_PIXEL(image2);
   gl_FragColor = (1-blendType) * min(color1, color2) + blendType * max(color1, color2);
}
