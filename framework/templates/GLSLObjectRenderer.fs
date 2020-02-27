/*{
	"ISFVSN":"2.0",
	"TYPE":"OBJECT_RENDER"
}*/

#include <VuoSurfaceUnlit.glsl>

void shadeSurface(inout Surface surface)
{
	surface.albedo = vec3(.3,.4,.5);
	surface.alpha = 1;
}
