// BabyKeySmash end-of-playtime scene ( --minutes N ) : a calm night sky with
// twinkling stars and a real photograph of the Moon , shaded with the actual
// lunar phase of tonight ( iMoonPhase : 0=new , 0.25=first quarter ,
// 0.5=full , 0.75=last quarter ) so the moon on screen matches the one
// outside the window .
// iChannel0 = audio texture , iChannel1 = the moon photograph
// Photo : FullMoon2010.jpg by Gregory H. Revera , CC BY-SA 3.0 , Wikimedia

float hash(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }

void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    vec2 uv = fragCoord/iResolution.xy;
    vec2 p  = (fragCoord - 0.5*iResolution.xy)/iResolution.y;

    // deep night gradient
    vec3 col = mix(vec3(0.01,0.01,0.06),vec3(0.04,0.03,0.12),uv.y);

    // twinkling stars
    vec2 cell = floor(fragCoord/24.0);
    float star = step(0.97,hash(cell));
    float twinkle = 0.5 + 0.5*sin(iTime*1.5 + hash(cell+7.0)*20.0);
    vec2 inCell = fract(fragCoord/24.0)-0.5;
    col += star * twinkle * smoothstep(0.15,0.0,length(inCell)) * vec3(0.9,0.9,0.7);

    // ---- the Moon ----
    vec2  moonPos = vec2(0.30,0.10);
    float moonRadius = 0.22;
    vec2  d  = (p-moonPos)/moonRadius;   // -1..1 across the disc
    float r2 = dot(d,d);
    float r  = sqrt(r2);

    // the near side of a sphere : the photo is an orthographic view of the
    // tidally locked near side , so the disc maps onto it one to one
    vec3 normal = vec3(d,sqrt(max(0.0,1.0-r2)));
    vec3 photo  = texture(iChannel1,clamp(d*0.5+0.5,0.0,1.0)).rgb;

    // sunlight direction for this phase , waxing lights the right hand side
    float ang = 6.2831853*iMoonPhase;
    vec3 sunDir = vec3(sin(ang),0.0,-cos(ang));
    float lit = smoothstep(0.0,0.10,dot(normal,sunDir));   // soft terminator

    // earthshine : the unlit part stays faintly visible , exactly like the
    // real "old moon in the new moon's arms"
    vec3 shaded = photo*vec3(1.00,0.97,0.88)*(lit + 0.07);

    // soft halo , stronger the fuller the moon is
    float illuminatedFraction = 0.5-0.5*cos(ang);
    col += vec3(0.30,0.30,0.22)*smoothstep(2.4,1.0,r)*0.16*(0.25+illuminatedFraction);

    float disc = smoothstep(1.0,0.985,r2);   // antialiased limb
    col = mix(col,shaded,disc);

    fragColor = vec4(col,1.0);
}
