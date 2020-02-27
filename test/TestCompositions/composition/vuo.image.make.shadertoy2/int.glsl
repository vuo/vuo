// Ensures that integer bitwise operators, uint, uvec2, and the unsigned-int type specifier (via EXT_gpu_shader4) are available.
void mainImage(out vec4 out_color, vec2 fragCoord)
{
	uvec2 c = uvec2(uint(gl_FragCoord.x) & uint(gl_FragCoord.y));
	float f = float(c.x & c.y);
	out_color = vec4(f,f,f,1U);
}
