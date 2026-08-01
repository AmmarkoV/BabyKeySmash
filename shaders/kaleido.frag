// BabyKeySmash alternate webcam effect : slowly rotating kaleidoscope of the
// live camera image ( iChannel0 = webcam , iChannel1 = audio texture )

void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    vec2 p = (fragCoord - 0.5*iResolution.xy)/iResolution.y;

    float bass = texture(iChannel1,vec2(0.05,0.25)).x;

    float angle = atan(p.y,p.x) + iTime*0.2;
    float radius = length(p) * (1.0 - 0.2*bass);

    // fold the angle into a mirrored wedge -> kaleidoscope
    float wedge = 3.14159265/4.0;
    angle = mod(angle,2.0*wedge);
    angle = abs(angle-wedge);

    vec2 uv = 0.5 + radius*vec2(cos(angle),sin(angle));
    vec3 cam = texture(iChannel0,fract(uv)).rgb;

    // gentle color cycling towards the rim
    cam *= 0.8 + 0.4*cos(radius*6.0 - iTime + vec3(0.0,2.0,4.0));

    fragColor = vec4(cam,1.0);
}
