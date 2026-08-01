// BabyKeySmash calm background ( --simplebg , or --background calm ) :
// a quiet gradient between deep blue and black that breathes very slowly .
// Made for winding down , for a dark room , or for a child who finds the
// animated effects too much .
// iChannel0 = audio texture

void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    vec2 uv = fragCoord/iResolution.xy;
    vec2 p  = (fragCoord - 0.5*iResolution.xy)/iResolution.y;

    // deep blue near the bottom fading up into almost black
    vec3 deepBlue = vec3(0.05,0.16,0.38);
    vec3 night    = vec3(0.01,0.02,0.07);
    vec3 col = mix(deepBlue,night,smoothstep(0.0,1.0,uv.y));

    // one slow breath every twenty seconds or so
    col *= 0.88 + 0.12*(0.5+0.5*sin(iTime*0.3));

    // a wide soft glow low on the screen , drifting gently sideways
    vec2 glowPos = vec2(0.6*sin(iTime*0.07),-0.35);
    float d = length((p-glowPos)*vec2(0.35,1.0));
    col += vec3(0.06,0.12,0.20)*smoothstep(0.9,0.0,d);

    // barely there answer to sound , just enough to feel alive
    float level = texture(iChannel0,vec2(0.05,0.25)).x;
    col += vec3(0.02,0.04,0.07)*level;

    // soft vignette , kept wide so it suits ultrawide multi monitor setups
    vec2 v = p*vec2(0.45,1.0);
    col *= 1.0 - 0.35*dot(v,v);

    fragColor = vec4(col,1.0);
}
