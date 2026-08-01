// BabyKeySmash background : colorful plasma that pulses with the microphone
// ShaderToy-compatible : iChannel0 is an audio texture ( row 0 spectrum ,
// row 1 waveform , like shadertoy.com soundcloud/mic inputs ) .
// Inspired by microphone visualizations such as shadertoy.com/view/4sjfzm ;
// any single-pass ShaderToy shader body can replace this file.

void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    vec2 p = (fragCoord - 0.5*iResolution.xy)/iResolution.y;
    vec2 m = (iMouse.xy  - 0.5*iResolution.xy)/iResolution.y;

    float bass = texture(iChannel0,vec2(0.05,0.25)).x;
    float mid  = texture(iChannel0,vec2(0.30,0.25)).x;

    float t = iTime*0.4;
    float d = length(p-m);
    float swirl = 0.5*sin(6.0*d - iTime*2.0);

    float v = 0.0;
    v += sin(p.x*4.0 + t + swirl);
    v += sin(p.y*3.0 - t*1.3);
    v += sin((p.x+p.y)*3.0 + t*0.7);
    v += sin(length(p)*8.0 - t*2.0 - bass*6.0);

    vec3 col = 0.5 + 0.5*cos(vec3(0.0,2.1,4.2) + v*1.5 + iTime*0.2);
    col *= 0.55 + 0.9*bass + 0.4*mid;

    // waveform ring dancing around the mouse position
    float angle = atan(p.y-m.y,p.x-m.x)/6.2831853 + 0.5;
    float wave = texture(iChannel0,vec2(angle,0.75)).x - 0.5;
    float ring = smoothstep(0.02,0.0,abs(d - (0.22+0.30*bass) - wave*0.15));
    col += ring*vec3(1.0,0.9,0.6);

    fragColor = vec4(col,1.0);
}
