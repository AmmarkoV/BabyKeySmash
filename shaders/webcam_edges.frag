// BabyKeySmash webcam effect : posterized camera image with rainbow edge glow
// ShaderToy-compatible : iChannel0 is the live ( mirrored ) webcam image ,
// iChannel1 is the audio texture .
// A stand-in for shaders like shadertoy.com/view/cll3zf which can be pasted
// over this file once fetched manually ( shadertoy blocks scripted downloads ).

float luma(vec2 uv) { return dot(texture(iChannel0,uv).rgb,vec3(0.299,0.587,0.114)); }

void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    vec2 uv = fragCoord/iResolution.xy;
    vec3 cam = texture(iChannel0,uv).rgb;

    float bass = texture(iChannel1,vec2(0.05,0.25)).x;

    // Sobel edge detection on luminance
    vec2 e = 2.0/iResolution.xy;
    float tl=luma(uv+vec2(-e.x, e.y)); float tc=luma(uv+vec2(0.0, e.y)); float tr=luma(uv+vec2(e.x, e.y));
    float ml=luma(uv+vec2(-e.x, 0.0));                                   float mr=luma(uv+vec2(e.x, 0.0));
    float bl=luma(uv+vec2(-e.x,-e.y)); float bc=luma(uv+vec2(0.0,-e.y)); float br=luma(uv+vec2(e.x,-e.y));
    float gx = (tr+2.0*mr+br) - (tl+2.0*ml+bl);
    float gy = (tl+2.0*tc+tr) - (bl+2.0*bc+br);
    float edge = clamp(length(vec2(gx,gy))*2.0,0.0,1.0);

    // Posterize the camera colors into cartoon bands
    vec3 posterized = floor(cam*4.0+0.5)/4.0;

    // Rainbow colored edges that cycle over time and thump with the bass
    vec3 rainbow = 0.5 + 0.5*cos(iTime + uv.xyx*4.0 + vec3(0.0,2.0,4.0));
    vec3 col = posterized*(0.75+0.5*bass) + edge*rainbow*(1.5+2.0*bass);

    fragColor = vec4(col,1.0);
}
