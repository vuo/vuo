
varying vec2 texcoord0;
varying vec2 texcoord1;
varying vec2 texcoord2;
varying vec2 texcoord3;
varying vec2 texcoord4;
varying vec2 texcoord5;
varying vec2 texcoord6;
varying vec2 texcoord7;

void main()
{
	vv_vertShaderInit();
	// perform standard transform on vertex
	//gl_Position = ftransform();

	// transform texcoord        
	vec2 texcoord = vec2(vv_FragNormCoord[0],vv_FragNormCoord[1]);

	// get sample positions
	texcoord0 = texcoord + vec2(-amount, -amount);
	texcoord1 = texcoord + vec2( 0,      -amount);
	texcoord2 = texcoord + vec2( amount, -amount);
	texcoord3 = texcoord + vec2(-amount,  0);
	texcoord4 = texcoord + vec2( amount,  0);
	texcoord5 = texcoord + vec2(-amount,  amount);
	texcoord6 = texcoord + vec2( 0,       amount);
	texcoord7 = texcoord + vec2( amount,  amount);
}
