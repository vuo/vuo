void main()
{
    vec4 fragColor;
    fragColor.w = 1.;
    mainImage(fragColor, (gl_FragCoord.xy+ifFragCoordOffsetUniform));
    gl_FragColor = fragColor;
}
