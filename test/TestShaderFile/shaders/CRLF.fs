/*
{
  "INPUTS": [
    {
      "NAME": "inputImage",
      "TYPE": "image"
    },
  ]
}
*/

void main(void) {
  gl_FragColor = IMG_THIS_PIXEL(inputImage);
}
