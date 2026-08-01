// BabyKeySmash alternate background : gentle cartoon sea with sun , waves
// pulse with the microphone ( iChannel0 = audio texture )

void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    vec2 uv = fragCoord/iResolution.xy;

    float bass = texture(iChannel0,vec2(0.05,0.25)).x;
    float mid  = texture(iChannel0,vec2(0.30,0.25)).x;

    // sky gradient
    vec3 col = mix(vec3(0.45,0.75,1.00),vec3(0.80,0.95,1.00),uv.y);

    // sun following the mouse a little
    vec2 m = iMouse.xy/iResolution.xy;
    vec2 sunPos = vec2(0.5+0.35*(m.x-0.5),0.8);
    float sunDist = length((uv-sunPos)*vec2(iResolution.x/iResolution.y,1.0));
    col += vec3(1.0,0.9,0.4)*smoothstep(0.09,0.05,sunDist);
    col += vec3(1.0,0.8,0.3)*0.35*smoothstep(0.30,0.0,sunDist);

    // layered waves , louder sound = taller waves
    float amp = 0.015 + 0.05*bass;
    float horizon = 0.45;
    int i;
    for (i=0; i<4; i++)
    {
        float fi = float(i);
        float level = horizon - fi*0.12;
        float wave = level
                   + amp*sin(uv.x*(8.0+fi*3.0) + iTime*(1.0+fi*0.4))
                   + amp*0.6*sin(uv.x*(15.0-fi*2.0) - iTime*(1.3+fi*0.2) + mid*4.0);
        if (uv.y < wave)
        {
            vec3 water = mix(vec3(0.10,0.45,0.75),vec3(0.05,0.25,0.55),fi/4.0);
            water += 0.10*smoothstep(wave-0.01,wave,uv.y); // foam line
            col = water;
        }
    }

    fragColor = vec4(col,1.0);
}
