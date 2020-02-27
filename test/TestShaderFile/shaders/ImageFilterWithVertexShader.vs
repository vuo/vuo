#define PI 3.1415926535

varying float _wavelength;
varying float _angle;
varying float _phase;

void main(void)
{
	isf_vertShaderInit();

	_wavelength = (abs(wavelength) < 0.000001 ? 0.000001 : wavelength) * PI * 2.;
	_angle = angle * PI/180.;
	_phase = phase * PI*2.;
}
