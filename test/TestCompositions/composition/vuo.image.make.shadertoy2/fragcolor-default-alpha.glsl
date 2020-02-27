// Shadertoy's `main()` sets fragColor.a before passing it to `out vec4 fragColor`,
// but some GPUs ignore the value passed in to an `out`-only argument.
// Vuo translates it to `inout`.
// Many popular Shadertoy shaders rely on this behavior,
// such as https://www.shadertoy.com/view/4tVBDz
void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
	fragColor.rgb = vec3(.4,.5,.6);
}
