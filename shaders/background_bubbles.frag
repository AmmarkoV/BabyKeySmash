// BabyKeySmash background option : underwater scene with rising bubbles ,
// louder sounds make bigger faster bubbles ( iChannel0 = audio texture )

float hash(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }

void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    vec2 uv = fragCoord/iResolution.xy;

    float bass = texture(iChannel0,vec2(0.05,0.25)).x;
    float mid  = texture(iChannel0,vec2(0.30,0.25)).x;

    // deep water gradient with light coming from the top
    vec3 col = mix(vec3(0.00,0.15,0.35),vec3(0.10,0.55,0.75),uv.y);

    // wavy light rays
    float ray = sin(uv.x*20.0 + sin(uv.y*4.0+iTime)*2.0 + iTime*0.5);
    col += vec3(0.10,0.20,0.20)*smoothstep(0.6,1.0,ray)*uv.y;

    // several layers of rising bubbles
    int layer;
    for (layer=0; layer<3; layer++)
    {
        float fl = float(layer);
        float columns = 10.0 + fl*6.0;
        vec2 grid = vec2(uv.x*columns , uv.y*columns*0.6 - iTime*(0.6+fl*0.4+bass*1.5));
        vec2 cell = floor(grid);
        float r = hash(cell);
        if (r>0.4)
        {
            vec2 inCell = fract(grid)-0.5;
            inCell.x += 0.2*sin(iTime*2.0+r*20.0);   // wobble sideways
            float size = (0.08+0.20*r)*(0.7+0.8*bass+0.4*mid);
            float bubble = length(inCell);
            float body = smoothstep(size,size*0.8,bubble);
            float rim  = smoothstep(size*0.55,size*0.95,bubble);
            col += body*rim*vec3(0.5,0.8,0.9)*(0.35+0.35/(1.0+fl));
            // highlight dot
            col += smoothstep(size*0.35,0.0,length(inCell-vec2(size*0.3)))*0.25;
        }
    }

    fragColor = vec4(col,1.0);
}
