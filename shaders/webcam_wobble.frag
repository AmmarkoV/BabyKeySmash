// BabyKeySmash webcam option : funhouse mirror , the image wobbles like
// jelly and ripples harder when there is sound
// ( iChannel0 = webcam , iChannel1 = audio )

void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    vec2 uv = fragCoord/iResolution.xy;

    float bass = texture(iChannel1,vec2(0.05,0.25)).x;
    float amp = 0.02 + 0.06*bass;

    // layered wobbles , like a funhouse mirror made of jelly
    uv.x += amp*sin(uv.y*9.0 + iTime*2.2);
    uv.y += amp*sin(uv.x*7.0 - iTime*1.7);
    uv.x += amp*0.5*sin(uv.y*23.0 - iTime*3.1);

    // gentle chromatic split for a dreamy look
    vec2 split = vec2(0.004+0.01*bass,0.0);
    vec3 cam;
    cam.r = texture(iChannel0,clamp(uv+split,0.0,1.0)).r;
    cam.g = texture(iChannel0,clamp(uv,0.0,1.0)).g;
    cam.b = texture(iChannel0,clamp(uv-split,0.0,1.0)).b;

    // vignette so the wobbling edges stay pretty
    vec2 c = fragCoord/iResolution.xy - 0.5;
    cam *= 1.0 - 0.6*dot(c,c);

    fragColor = vec4(cam,1.0);
}
