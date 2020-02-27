/*{
	"ISFVSN":"2.0",
	"TYPE":"OBJECT_FILTER"
}*/

#include <VuoDeform.glsl>

vec3 deformVertex(in vec3 position)
{
   return sin(position);
}
