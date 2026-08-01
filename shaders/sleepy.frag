// BabyKeySmash end-of-playtime scene ( --minutes N ) : a calm dark night sky
// with a big friendly moon and twinkling stars , signalling sleep time

float hash(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }

void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    vec2 uv = fragCoord/iResolution.xy;
    vec2 p = (fragCoord - 0.5*iResolution.xy)/iResolution.y;

    // deep night gradient
    vec3 col = mix(vec3(0.01,0.01,0.06),vec3(0.04,0.03,0.12),uv.y);

    // twinkling stars
    vec2 cell = floor(fragCoord/24.0);
    float star = step(0.97,hash(cell));
    float twinkle = 0.5 + 0.5*sin(iTime*1.5 + hash(cell+7.0)*20.0);
    vec2 inCell = fract(fragCoord/24.0)-0.5;
    col += star * twinkle * smoothstep(0.15,0.0,length(inCell)) * vec3(0.9,0.9,0.7);

    // big moon , breathing very slowly
    vec2 moonPos = vec2(0.55,0.12);
    float moon = length(p-moonPos) - (0.18 + 0.004*sin(iTime*0.5));
    col = mix(vec3(0.92,0.92,0.80),col,smoothstep(0.0,0.01,moon));
    // craters
    col -= 0.08*smoothstep(0.03,0.0,length(p-moonPos-vec2(0.05,0.04)))*step(moon,0.0);
    col -= 0.06*smoothstep(0.025,0.0,length(p-moonPos+vec2(0.06,0.03)))*step(moon,0.0);
    // soft glow
    col += vec3(0.35,0.35,0.25)*smoothstep(0.35,0.0,moon)*0.25;

    fragColor = vec4(col,1.0);
}
